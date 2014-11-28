#include <stdio.h>
#include <string.h>
#include <stdlib.h>

FILE *ifp,*ofp;


void writeError() {
        perror("ppnstrip");
        exit(-1);
}

void skipWhite(int output) {
        //skip spaces and tabs
        int c;
        while((c=getc(ifp))==' ' || c=='\t') {
                if(output) {
                        if(putc(c,ofp)==EOF) writeError();
                }
        }
        ungetc(c,ifp);
}

void skipWhiteNL(int output) {
        //skip spaces, tabs and newliens
        int c;
        while((c=getc(ifp))==' ' || c=='\t' || c=='\n') {
                if(output) {
                        if(putc(c,ofp)==EOF) writeError();
                }
        }
        ungetc(c,ifp);
}

void passThroughPPD() {
        //pass a preprocessor directive line (and backslashed lines) through
        int lastChar=0;
        for(;;) {
                int c=getc(ifp);
                if(c==EOF) break;
                putc(c,ofp);
                if(c=='\n') {
                        //could be end PPD
                        if(lastChar!='\\') {
                                //yes, it's the end
                                return;
                        }
                }
                lastChar=c;
        }
}

void passThroughTypedef() {
        int c;
        while((c=getc(ifp))!=EOF) {
                putc(c,ofp);
                if(c=='#') {
                        passThroughPPD();
                } else {
                        if(c==';')
                                break; //end of typedef reached
                }
        }
}


struct token {
        char s[256];
};

inline int isLegalFirstChar(int c) {
        return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c=='_');
}
inline int isLegalSecondChar(int c) {
        return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' & c<'9') || (c=='_');
}

int getToken(token &tk) {
        int c=getc(ifp);
        if(c==EOF) return EOF;
        if(isLegalFirstChar(c)) {
                //a word
                char *p=tk.s;
                while(isLegalSecondChar(c)) {
                        *p++=(char)c;
                        c=getc(ifp);
                }
                *p='\0';
                ungetc(c,ifp);
        } else if(c=='.') {
                //could be a "..." parameter
                char *p=tk.s;
                while(c=='.') {
                        *p++=(char)c;
                        c=getc(ifp);
                }
                *p='\0';
                ungetc(c,ifp);
        } else {
                tk.s[0]=(char)c;
                tk.s[1]='\0';
        }
        return 0;
}

void putToken(const token &tk) {
        if(fputs(tk.s,ofp)==EOF) {
                perror("ppnstrip");
                exit(-1);
        }
}

//prototype decl::=  param [ ',' param ...] )
//param::=  <type> [modifer ...] [name]

void stripPrototype() {
        //compress  type [modifiers...] name ,
        //into      type [modifiers] ,
        //Eg:
        //  WinFillRect(HPS hps, PRECTL prcl, LONG lColor)
        //  WinFillRect(HPS hps, RECTL * prcl, LONG lColor)
        //->
        //  WinFillRect(HPS,PRECTL,LONG)
        //  WinFillRect(HPS,RECTL *,LONG)
        token tk1,tk2;
        do {    //for each parameter

                //pass through type
                skipWhiteNL(0);
                if(getToken(tk1)==EOF) return;
                putToken(tk1);

                //get the next two tokens
                skipWhiteNL(0);
                if(getToken(tk1)==EOF) return;
                if(tk1.s[0]!=',' && tk1.s[0]!=')') {
                        getToken(tk2);
                        while(tk2.s[0]!=',' && tk2.s[0]!=')') {
                                putc(' ',ofp);
                                putToken(tk1);
                                tk1=tk2;
                                skipWhiteNL(0);
                                if(getToken(tk2)==EOF) return;
                        }
                
                        //Was the last token a modifer or a name?
                        if(isLegalFirstChar(tk1.s[0])) {
                                //a name - must be a parameter name
                        } else {
                                //a type modifier (such as * or &)
                                putToken(tk1);
                        }
                } else {
                        tk2=tk1;
                }
                //output the comma or rparen
                putToken(tk2);
        } while(tk2.s[0]!=')');
}

void stripPPN() {
        token tk;
        for(;;) {
                skipWhiteNL(1);
                if(getToken(tk)==EOF) return;
                if(tk.s[0]=='#') {
                        putToken(tk);
                        passThroughPPD();
                } else if(strcmp(tk.s,"typedef")==0) {
                        putToken(tk);
                        passThroughTypedef();
                } else {
                        putToken(tk);
                        if(tk.s[0]=='(')
                                stripPrototype();
                }
        }
}




int main(int argc, char *argv[]) {
        if(argc==2 && argv[1][0]=='-' && argv[1][1]=='p' && argv[1][2]=='\0') {
                //pipe
                ifp=stdin;
                ofp=stdout;
        } else {
                if(argc!=3) {
                        printf("Usage: ppnstrip <infile> <outfile>\n");
                        return -1;
                }
                ifp=fopen(argv[1],"rt");
                if(!ifp) {
                        fprintf(stderr,"ppnstrip: could not open '%s'\n",argv[1]);
                        return -1;
                }
                ofp=fopen(argv[2],"wt");
                if(!ofp) {
                        fprintf(stderr,"ppnstrip: could not create '%s'\n",argv[2]);
                        return -1;
                }
        }

        setvbuf(ifp,NULL,_IOFBF,4096);
        setvbuf(ofp,NULL,_IOFBF,4096);
        stripPPN();

        fclose(ifp);
        if(fclose(ofp)) writeError();

        return 0;
}

