#include <stdio.h>

void stripSpaces(const char *inbuf, char *outbuf) {
        //find first non-blank in string
        const char *p=inbuf;
        while(*p==' ' || *p=='\t')
                p++;
        //find end of string
        const char *endptr=p;
        while(*endptr!='\0')
                endptr++;
        endptr--;
        //find last non-blank
        while(endptr>=p && (*endptr==' ' || *endptr=='\t' || *endptr=='\n'))
                endptr--;

        while(p<=endptr) {
                if(p[0]==' ' && p[1]==' ')
                        p++;
                else
                        *outbuf++=*p++;
        }
        *outbuf++='\n';
        *outbuf='\0';
};

int main(int argc, char *argv[]) {
        FILE *ifp,*ofp;
        if(argc==2 && argv[1][0]=='-' && argv[1][1]=='p' && argv[1][2]=='\0') {
                //pipe
                ifp=stdin;
                ofp=stdout;
        } else {
                if(argc!=3) {
                        printf("Usage: spcstrip <infile> <outfile>\n");
                        return -1;
                }
                ifp=fopen(argv[1],"rt");
                if(!ifp) {
                        fprintf(stderr,"spcstrip: could not open '%s'\n",argv[1]);
                        return -1;
                }
                ofp=fopen(argv[2],"wt");
                if(!ofp) {
                        fprintf(stderr,"spcstrip: could not create '%s'\n",argv[2]);
                        return -1;
                }

        }

        setvbuf(ifp,NULL,_IOFBF,4096);
        setvbuf(ofp,NULL,_IOFBF,4096);

        char oldLine[512];
        char newLine[512];
        while(fgets(oldLine,512,ifp)) {
                stripSpaces(oldLine,newLine);
                if(newLine[0]!='\n')    //only non-empty lines will be output
                        fputs(newLine,ofp);
        }

        fclose(ifp);
        if(fclose(ofp)) {
                perror("spcstrip");
                return -1;
        }

        return 0;
}

