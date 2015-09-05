function replace(ifile, ofile, proto_name) {
    printf "" > ofile;
    while((getline line < ifile) > 0) {
        gsub(/\$UPROTO\$/, proto_name, line);
        print line >> ofile;
    }
}

/^BEGIN/ {
    proto=$2;
    protofile="gen/"proto".h";
    printf "" > protofile;
    print "#ifndef __"toupper(proto)"_H__" > protofile;
    print "#define __"toupper(proto)"_H__\n" > protofile;
}

/^REQUEST/ {
    print "typedef struct "proto"_request_s "proto "_request_t;" > protofile;
    print "struct "proto"_request_s {" > protofile;
}

/^}/ {
    print "};\n" > protofile;
}

/^RESPONSE/ {
    print "typedef struct "proto"_response_s "proto"_response_t;" > protofile;
    print "struct "proto"_response_s {" > protofile;
}

/^END/ {
    print "#endif" > protofile;
}

/;$/ {print $0 > protofile;}

END {
    replace("../userver/include/userver.h", "gen/"proto"-server.h", proto);
    replace("../userver/include/uclient.h", "gen/"proto"-client.h", proto);
    replace("../userver/src/uclient.c", "gen/"proto"-client.c", proto);
    replace("../userver/src/userver.c", "gen/"proto"-server.c", proto);
}
