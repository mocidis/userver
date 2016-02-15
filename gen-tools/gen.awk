function replace(ifile, ofile, proto_name) {
    while( (getline line < ifile) > 0 ) {
        gsub(/\$UPROTO\$/, proto_name, line);
        print line >> ofile;
    }
}
function gen_parse_request_specific(messages, fieldtypes, sfile, hfile) {
    for (m in messages) {
    print "void "proto"_parse_"m"(json_object *jobj, char *buffer, int len, "proto"_request_t *request) {" >> sfile;
    print "    json_object_object_foreach(jobj, key, val) {" >> sfile;
        has_else = 0;
        for (p in fieldtypes) {
            if( fieldrequests[p] == m && p != "msg_id") {
                if( has_else == 0 ) {
                    has_else = 1;
    print "        if(strcmp(key,\"" p "\") == 0) {" >> sfile;
                }
                else {
    print "        else if(strcmp(key,\"" p "\") == 0) {" >> sfile;
                }
                if (fieldtypes[p] == "char") {
    print "            strncpy(request->"fieldrequests[p]"."p", json_object_get_string(val), sizeof(request->"fieldrequests[p]"."p"));" >> sfile;
                }
                else {
    print "            request->"fieldrequests[p]"."p" = json_object_get_"fieldtypes[p]"(val);" >> sfile;
                }
    print "        }" >> sfile;
            }
        }
    print "    }" >> sfile;
    print "}" >> sfile;
    }
}
function gen_parse_request(messages, fieldtypes, sfile, hfile) {
    print "void "proto"_parse_request(char *buffer, int len, "proto"_request_t *request);" >> hfile;
    print "void "proto"_parse_request(char *buffer, int len, "proto"_request_t *request) {" >> sfile;
    print "    json_object *jobj, *jvalue;" >> sfile;
    print "    int msg_id;" >> sfile;
    print "    buffer[len] = '\\0';" >> sfile;
    print "    jobj = json_tokener_parse(buffer);" >> sfile;
    print "    json_object_object_get_ex(jobj, \"msg_id\", &jvalue);">> sfile;
    print "    msg_id = json_object_get_int(jvalue);" >> sfile;
    print "    request->msg_id = msg_id;" >> sfile;
    print "    switch(msg_id) {" >> sfile;
    
    for (m in messages) {
    print "    case "toupper(m)":" >> sfile;
    print "        "proto"_parse_"m"(jobj, buffer, len, request);" >> sfile;
    print "        break;" >> sfile;
    }

    print "    default:" >> sfile;
    print "        fprintf(stderr, \""proto"_parse_request(): Invalid msg_id; %d\\n\", msg_id);" >> sfile;
    print "        exit(-1);" >> sfile;
    print "    }" >> sfile;
    print "}" >> sfile;
}
function gen_build_request_specific(messages, fieldtypes, sfile, hfile) {
    for (m in messages) {
    print "void "proto"_build_"m"(json_object *jobj, "proto"_request_t *request) {" >> sfile;
        for (p in fieldtypes) {
            if (fieldrequests[p] == m) {
                if(fieldtypes[p] == "char") {
    print "    json_object_object_add(jobj, \""p"\", json_object_new_string(request->"fieldrequests[p]"." p "));" >> sfile;
                }
                else {
    print "    json_object_object_add(jobj, \""p"\", json_object_new_" fieldtypes[p] "(request->"fieldrequests[p]"." p "));" >> sfile;
                }
            }
        }
    print "}" >> sfile;
    }
}
function gen_build_request(messages, fieldtypes, sfile, hfile) {
    print "void "proto"_build_request(char *buffer, int len, "proto"_request_t *request);" >> hfile;
    print "void "proto"_build_request(char *buffer, int len, "proto"_request_t *request) {" >> sfile;
    print "    json_object *jobj;" >> sfile;
    print "    jobj = json_object_new_object();" >> sfile;
    print "    json_object_object_add(jobj, \"msg_id\", json_object_new_int(request->msg_id));" >> sfile;
    print "    switch(request->msg_id) {" >> sfile;

    for (m in messages) {
    print "    case " toupper(m) ":" >> sfile;
    print "        "proto"_build_"m"(jobj, request);" >> sfile;
    print "        break;" >> sfile;
    }

    print "    default:" >> sfile;
    print "        fprintf(stderr, \""proto"_build_request():Invalid msg_id: %d\\n\", request->msg_id);" >> sfile;
    print "        exit(-1);" >> sfile;

    print "    }" >> sfile;
    print "    strncpy(buffer, json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PLAIN), len);" >> sfile;
    print "    json_object_put(jobj);" >> sfile;
    print "}" >> sfile;
}
BEGIN {
}
/^BEGIN/ {
    proto=$2;
    protofile="gen/"proto".h";
    printf "" > protofile;
    print "#ifndef __"toupper(proto)"_H__" >> protofile;
    print "#define __"toupper(proto)"_H__\n" >> protofile;

    print "typedef struct "proto"_request_s "proto "_request_t;" >> protofile;
    print "struct "proto"_request_s {" >> protofile;
    print "    int msg_id;" >> protofile;
    print "    union {" >> protofile;
}
/^ENCRYPTED/ {
    is_encrypted=$2;
}

/^REQUEST/ {
    req = $2;
    messages[req] = $3;
    print "        struct {" >> protofile;
}

/^}/ {
    print "        } " req ";\n" >> protofile;
}

/^END/ {
    print "    };" >> protofile;
    print "};\n" >> protofile;
    for (m in messages) {
        print "#define " toupper(m) " " messages[m] >> protofile;
    }
    if( is_encrypted == "ON" ) {
        print "#define USERVER_ENCRYPTED" >> protofile;
    }
    print "#define "
    print "#endif" >> protofile;
}

/^[ \t]*PARAM/ {
    fieldtypes[$2] = $3;
    fieldrequests[$2] = req;
    print "            "$3, $2$4";" >> protofile;
}

END {
    userver_h = "gen/"proto"-server.h";
    userver_c = "gen/"proto"-server.c";
    uclient_h = "gen/"proto"-client.h";
    uclient_c = "gen/"proto"-client.c";
    print "#ifndef __" toupper(proto) "_SERVER_H__" > userver_h;
    print "#define __" toupper(proto) "_SERVER_H__" >> userver_h;
#    print "#include \"" proto ".h\"" >> userver_h;

    print "#ifndef __" toupper(proto) "_CLIENT_H__" > uclient_h;
    print "#define __" toupper(proto) "_CLIENT_H__" >> uclient_h;
#    print "#include \"" proto ".h\"" >> uclient_h;

    replace(base_dir"/include/userver.h", userver_h, proto);
    replace(base_dir"/include/uclient.h", uclient_h, proto);
    replace(base_dir"/src/userver.c", userver_c, proto);
    replace(base_dir"/src/uclient.c", uclient_c, proto);

#    replace("../userver/include/userver.h", userver_h, proto);
#    replace("../userver/include/uclient.h", uclient_h, proto);
#    replace("../userver/src/userver.c", userver_c, proto);
#    replace("../userver/src/uclient.c", uclient_c, proto);

    gen_parse_request_specific(messages, fieldtypes, userver_c, userver_h);
    gen_parse_request(messages, fieldtypes, userver_c, userver_h);
    gen_build_request_specific(messages, fieldtypes, uclient_c, uclient_h);
    gen_build_request(messages, fieldtypes, uclient_c, uclient_h);

    printf "#endif" >> userver_h;
    printf "#endif" >> uclient_h;
}
