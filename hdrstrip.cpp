#define INCL_DOSQUEUES
#define INCL_DOSFILEMGR
#define INCL_DOSPROCESS

#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define HF_STDIN  ((HFILE)0)
#define HF_STDOUT ((HFILE)1)


APIRET ExecPipe(const char *pgmname, const char *cmdline, HFILE stdInput, HFILE stdOutput, PID *pid) {
        //save stdin&stdout
        HFILE hfSaveInput=-1;
        HFILE hfSaveOutput=-1;
        DosDupHandle(HF_STDIN,&hfSaveInput);
        DosDupHandle(HF_STDOUT,&hfSaveOutput);
        //redirect stdin&stdout to stdInput&stdOutput
        {
         HFILE hfNewIn=HF_STDIN,hfNewOut=HF_STDOUT;
        
         DosDupHandle(stdInput,&hfNewIn);
         DosDupHandle(stdOutput,&hfNewOut);
        }


        char szFailName[CCHMAXPATH];

        RESULTCODES res;
        APIRET rc;
        rc = DosExecPgm(szFailName, sizeof(szFailName), EXEC_ASYNCRESULT, cmdline, NULL, &res, pgmname);
        *pid=res.codeTerminate;
               
        //restore stdin&stdout
        {
         HFILE hfNewIn=HF_STDIN,hfNewOut=HF_STDOUT;
         DosDupHandle(hfSaveInput,&hfNewIn);
         DosDupHandle(hfSaveOutput,&hfNewOut);
         DosClose(hfSaveInput);
         DosClose(hfSaveOutput);
        }

        if(rc) {
                fprintf(stderr,"hdrstrip: could not start '%s' (rc=%d)\n",pgmname,rc);
        }
        return rc;
}


int WaitPipe(PID pid) {
        APIRET rc;
        RESULTCODES res;
        PID pidIKnowThat;
        rc=DosWaitChild(DCWA_PROCESS,DCWW_WAIT,&res,&pidIKnowThat,pid);
        if(rc!=0) {
                fprintf(stderr,"hdrstrip: could not retrieve child proccess return code (rc=%d)\n",rc);
                return -1;
        }
        if(res.codeTerminate!=TC_EXIT)
                return -1;
        return res.codeResult;
}


int stripHeader(HFILE hfInFile, HFILE hfOutFile)
{
        APIRET rc;

        PID pidCommentStripper, pidSpaceStripper, pidPPNStripper;
        HFILE hfCommentOut,hfSpaceIn,hfSpaceOut,hfPPNIn;

        //create the pipes
        rc=DosCreatePipe(&hfSpaceIn,&hfCommentOut,4096);
        if(rc) {
                fprintf(stderr,"hdrstrip: could not create pipe (rc=%d)\n",rc);
                return -1;
        }
        rc=DosCreatePipe(&hfPPNIn,&hfSpaceOut,4096);
        if(rc) {
                fprintf(stderr,"hdrstrip: could not create pipe (rc=%d)\n",rc);
                return -1;
        }

        //create the child processes

        rc=ExecPipe("cmtstrip.exe", "cmtstrip\0-p\0", hfInFile,  hfCommentOut, &pidCommentStripper);
        if(rc) return -2;
        DosClose(hfInFile);
        DosClose(hfCommentOut);

        rc=ExecPipe("spcstrip.exe", "spcstrip\0-p\0", hfSpaceIn, hfSpaceOut,   &pidSpaceStripper);
        if(rc) return -2;
        DosClose(hfSpaceIn);
        DosClose(hfSpaceOut);

        rc=ExecPipe("ppnstrip.exe", "ppnstrip\0-p\0", hfPPNIn,   hfOutFile,    &pidPPNStripper);
        if(rc) return -2;
        DosClose(hfPPNIn);
        DosClose(hfOutFile);


        //wait for the childs to finish
        int r1=WaitPipe(pidCommentStripper);
        int r2=WaitPipe(pidSpaceStripper);
        int r3=WaitPipe(pidPPNStripper);

        if(r1) {
                fprintf(stderr,"hdrstrip: commentstripper failed (r=%d)\n",r1);
                return -1;
        }
        if(r2) {
                fprintf(stderr,"hdrstrip: spacestripper failed (r=%d)\n",r2);
                return -1;
        }
        if(r3) {
                fprintf(stderr,"hdrstrip: prototypestripper failed (r=%d)\n",r3);
                return -1;
        }
        return 0;
}

int stripHeader(const char *inFileName, const char *outFileName) {
        APIRET rc;
        HFILE hfInFile,hfOutFile;
        ULONG iDontCare;
        rc = DosOpen(inFileName,
                     &hfInFile,
                     &iDontCare,
                     0,
                     0,
                     OPEN_ACTION_FAIL_IF_NEW|OPEN_ACTION_OPEN_IF_EXISTS,
                     OPEN_FLAGS_SEQUENTIAL|OPEN_SHARE_DENYWRITE|OPEN_ACCESS_READONLY,
                     0
                    );

        if(rc) {
                fprintf(stderr,"hdrstrip: could not open %s (rc=%d)\n",inFileName,rc);
                return -1;
        }

        rc = DosOpen(outFileName,
                     &hfOutFile,
                     &iDontCare,
                     0,
                     0,
                     OPEN_ACTION_CREATE_IF_NEW|OPEN_ACTION_REPLACE_IF_EXISTS,
                     OPEN_FLAGS_SEQUENTIAL|OPEN_SHARE_DENYREADWRITE|OPEN_ACCESS_WRITEONLY,
                     0
                    );
        if(rc) {
                fprintf(stderr,"hdrstrip: could not create %s (rc=%d)\n",outFileName,rc);
                DosClose(hfInFile);
                return -1;
        }

        int r=stripHeader(hfInFile,hfOutFile);
        if(r) {
                //clean up
                DosForceDelete(outFileName);
        }
        return r;
}

int stripHeader(const char *filename) {
        APIRET rc;
        char drive[_MAX_DRIVE], dir[_MAX_DIR],name[_MAX_FNAME],ext[_MAX_EXT];
        _splitpath(filename,drive,dir,name,ext);

        char backupFilename[_MAX_PATH];
        _makepath(backupFilename,drive,dir,name,".bak");

        rc = DosMove(filename,backupFilename);
        if(rc) {
                fprintf(stderr,"hdrstrip: could not rename %s to %s (rc=%d)\n",filename,backupFilename,rc);
                return -1;
        }

        int r=stripHeader(backupFilename,filename);
        if(r) {
                DosMove(backupFilename,filename);
        }
        return r;
}

int main(int argc, char *argv[]) {
        if(argc==2 && argv[1][0]=='-' && argv[1][1]=='p' && argv[1][2]=='\0') {
                return stripHeader(HF_STDIN,HF_STDOUT);
        } else {
                if(argc!=3) {
                        printf("Usage: hdrstrip <infile> [<outfile>|-b]\n");
                        return -1;
                }
                if(argv[2][0]=='-' && argv[2][1]=='b' && argv[2][2]=='\0') 
                        return stripHeader(argv[1]);
                else
                        return stripHeader(argv[1],argv[2]);
        }
}
