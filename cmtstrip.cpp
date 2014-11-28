#include <stdio.h>
#include <stdlib.h>

FILE *ifp,*ofp;

int cmtgetchar() {
        for(;;) {
                int c=getc(ifp);
                if(c==EOF) return EOF;
                if(c=='/') {
                        //could be a comment
                        c=getc(ifp);
                        if(c=='/') {
                                //C++ style comment
                                while(c!=EOF && c!='\n') {
                                        c=getc(ifp);
                                }
                        } else if(c=='*') {
                                //C style comment
                                for(;;) {
                                        c=getc(ifp);
                                        if(c==EOF) return EOF;  //premature eof
                                        if(c=='*') {
                                                //could be end of comment
                                                c=getc(ifp);
                                                if(c=='/') {
                                                        //yes, end of comment
                                                        break;
                                                } else {
                                                        //no
                                                        ungetc(c,ifp);
                                                }
                                        }
                                } 
                        } else {
                                //not a comment
                                ungetc(c,ifp);
                                return '/';
                        }
                } else
                        return c;
        }
}

void stripComments() {
        int c;
        while((c=cmtgetchar())!=EOF) {
                if(putc(c,ofp)==EOF) {
                        fprintf(stderr,"cmtstrip: could not write to output\n");
                        perror("cmtstrip");
                        exit(4);
                }
        }
}

void main(int argc, char *argv[]) {
        if(argc==2 && argv[1][0]=='-' && argv[1][1]=='p' && argv[1][2]=='\0') {
                //pipe
                ifp=stdin;
                ofp=stdout;
        } else {
                if(argc!=3) {
                        fprintf(stderr,"Usage: cmtstrip <infile> <outfile>\n");
                        exit(1);
                }
                ifp=fopen(argv[1],"rt");
                if(!ifp) {
                        fprintf(stderr,"cmtstrip: could not open '%s'\n",argv[1]);
                        exit(2);
                }
                ofp=fopen(argv[2],"wt");
                if(!ofp) {
                        fprintf(stderr,"cmtstrip: could not create '%s'\n",argv[2]);
                        exit(3);
                }

        }

        setvbuf(ifp,NULL,_IOFBF,4096);
        setvbuf(ofp,NULL,_IOFBF,4096);

        stripComments();

        fclose(ifp);
        if(fclose(ofp)) {
                fprintf(stderr,"cmtstrip: could not close output\n");
                perror("cmtstrip");
                exit(4);
        }


        exit(0);
}

