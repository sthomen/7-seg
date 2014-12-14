BEGIN {
	FS=""
	print \
"/* This code is automatically regenerated from " FILENAME " */\n\n"\
"struct {\n"\
"	char character;\n"\
"	char data[14];\n"\
"} segment_14[]={";
}

{
	for (i=1;i<NF+1;i++) {
		if ($i=="_") {
			line=line "0,";
		}
		if ($i=="#") {
			line=line "1,";
		}
	}
}
/-/ {
	print_line($2, line);
	line="";
}

END {
	print "};";

	print "#define SEGXIV_MAX " lines;
}

function print_line(char, line, comma) {
	print "\t{ '" char "', {" line "} },"
	lines++;
}
