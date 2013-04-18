#ifndef _REENTRANT 
#define _REENTRANT 
#endif 

#include <sys/types.h>	/* definierar typen pid_t */
#include <errno.h>		/* definierar bland annat perror() */
#include <stdio.h>		/* definierar bland annat stderr */
#include <stdlib.h>		/* definierar bland annat rand() och RAND_MAX, getenv() */
#include <unistd.h>		/* definierar bland annat pipe() */
#include <sys/wait.h>
#include <signal.h>		/* deinierar signalnamn med mera */
#define TRUE ( 1 )		/* den logiska konstanten TRUE */

#include <sys/stat.h>	/* definierar S_IREAD och S_IWRITE */
#include <fcntl.h>		/* definierar bland annat open() och O_RDONLY */

#define PIPE_READ 0
#define PIPE_WRITE 1


pid_t parent_pid; /* för parentprocessen */
pid_t child_pid; /* för child-processens PID vid fork() */

pid_t printenv_pid = -1;
pid_t grep_pid = -1;
pid_t sort_pid = -1;
pid_t pager_pid = -1;

void printenv(int pipeline[]);
void sort(int pipeline[], int pipeline2[]);
void handlePager(int pipeline[]);
void createPipe(int pipeline[]);
void waitForChild();
void closeFileDesc(int fd);
void grep(char **argv, int pipeline1[], int pipeline2[]);
void duplicate(int from, int to);

int main(int argc, char **argv, char **envp) {	
	parent_pid = (pid_t) getpid();
	
	/* skapa en pipe 1 */
	int stageOnePipe[2];	
	createPipe(stageOnePipe);
	
	/* ------------------Printenv pipe-------------------*/		
	/* skapa en child-process */
	printenv_pid = child_pid = fork();		
	if( 0 == child_pid ) {
		fprintf( stderr,"printenv (pid %ld) started\n", (long int) getpid() );
		printenv(stageOnePipe);  
	}
	
	/* kolla om fork() misslyckades */
	if( -1 == child_pid ) { perror( "Cannot fork()" ); exit( errno ); }
	
	/* stäng skrivsidan av pipen */
	closeFileDesc(stageOnePipe[PIPE_WRITE] );	
	
	
	/* ------------------Grep pipe-------------------*/		
	/* Om antalet parametrar är större än 1 så ska ett till pipelinesteg till grep läggas till */
	/* skapa pipe 2 */
	int stageTwoPipe[2];	
	int useStageTwoPipe = 0; //Indikerare om sort ska läsa från stageTwoPipe eller stageOnePipe.
	if(argc > 1) {
		fprintf( stderr,"Adding grep stage\n");
		useStageTwoPipe = 1;
		createPipe(stageTwoPipe);		
		
		/* skapa en child-process */
		grep_pid = child_pid = fork();		
		if( 0 == child_pid ) {
			fprintf( stderr,"grep (pid %ld) started\n", (long int) getpid() );
			grep(argv, stageOnePipe, stageTwoPipe);
		}
		
		/* kolla om fork() misslyckades */
		if( -1 == child_pid ) {perror( "Cannot fork()" ); exit( errno );}
		
		/* stäng onödiga fildeskriptors*/
		closeFileDesc(stageOnePipe[PIPE_READ] );
		closeFileDesc(stageTwoPipe[PIPE_WRITE] );		
	}
	
	/* skapa pipe 2 */
	int stageThreePipe[2];	
	createPipe(stageThreePipe);	
	
	/* ------------------Sort pipe-----------------------*/
	/* skapa en child-process */
	sort_pid = child_pid = fork();		
	if( 0 == child_pid ) {
		fprintf( stderr,"sort (pid %ld) started\n", (long int) getpid() );
		if( 1 == useStageTwoPipe )	sort(stageTwoPipe, stageThreePipe);
		else 						sort(stageOnePipe, stageThreePipe);		
	}
	
	/* kolla om fork() misslyckades */
	if( -1 == child_pid ) { perror( "Cannot fork()" ); exit( errno ); }
	
	/* stäng onödiga filedescriptors */
	if(1 == useStageTwoPipe)		closeFileDesc(stageTwoPipe[PIPE_READ] );	
	else 							closeFileDesc(stageOnePipe[PIPE_READ] );
	closeFileDesc(stageThreePipe[PIPE_WRITE]);	
	
	
	/* ------------------Pager pipe-----------------------*/
	/* skapa en child-process */
	pager_pid = child_pid = fork();		
	if( 0 == child_pid ) {
		fprintf( stderr,"pager (pid %ld) started\n", (long int) getpid() );
		handlePager(stageThreePipe);
	}
	
	/* kolla om fork() misslyckades */
	if( -1 == child_pid ) { perror( "Cannot fork()" ); exit( errno ); }
	
	/* stäng onödiga filedescriptors */
	closeFileDesc(stageThreePipe[PIPE_READ]);
	
	/* Vänta på barnen */
	if( 1 == useStageTwoPipe ) //Om grep har körts så måte vi vänta på ett extra barn
		waitForChild();	
	waitForChild();
	waitForChild();
	waitForChild();	
	
	return 0;
}


/**
 * Körs bara av childprocessen. Pipar utdata från printenv() till sort
 * genom att duplicera pipelinens skrivände till stdout
 */
void printenv(int pipeline[]) {

	/* stäng lässidan av pipen */
	closeFileDesc(pipeline[PIPE_READ] );	
	
	/* ersätt stdout med duplicerad skriv-ände på pipen */
	duplicate( pipeline[ PIPE_WRITE ], STDOUT_FILENO );	
	
	/* stäng lässidan av den dupade pipen */
	closeFileDesc(pipeline[PIPE_WRITE] );	
	
	execlp( "printenv", "printenv", (char *) 0 );
	
	/* exec returnerar bara om något fel har uppstått
	   och om exec returnerar så är returvärdet alltid -1 */
	   
	perror( "Cannot exec printenv" );
	exit( errno );
}

void grep(char **argv, int readPipeline[], int writePipeline[]) {	
	
	/* stäng onödiga pipes */
	closeFileDesc(writePipeline[PIPE_READ]);	
	
	/* Sätt ihop argumenten */
	argv[0] = "grep";		
	
	/* ersätt stdin med duplicerad läs-ände på läspipen */
	duplicate( readPipeline[ PIPE_READ ], STDIN_FILENO );	
	
	/* kan stänga den ursprungliga pipen */
	closeFileDesc(readPipeline[PIPE_READ] );

	/* ersätt stdout med duplicerad skriv-ände på skrivpipen */
	duplicate( writePipeline[ PIPE_WRITE ], STDOUT_FILENO );	
	
	/* kan stänga den ursprungliga pipen */
	closeFileDesc(writePipeline[PIPE_WRITE] );
	
	/* Söker i PATH efter filnamnet och tar som andra argumenetet en pekare till en nullterminerad string-array (argumenten) */
	execvp("grep", argv);
	
	perror( "Cannot exec grep" );
	exit(errno);
}


/**
 * Körs bara av childprocessen. Läser indatan från pipeenv() från readPipeLine
 * och skriver resultatet till den filedescriptorn som pekar på den temporära filen
 */
void sort(int readPipeline[], int writePipeline[]) {
	/* kan stänga onödiga pipes*/
	closeFileDesc(writePipeline[PIPE_READ]);	
	
	/* ersätt stdin med duplicerad läs-änden på readPipen */
	duplicate( readPipeline[ PIPE_READ ], STDIN_FILENO );	
	
	/* kan stänga den ursprungliga pipen */
	closeFileDesc(readPipeline[PIPE_READ] );	
	
	/* ersätt stdout med deplicerad skrivände på andra pipen */
	duplicate( writePipeline[PIPE_WRITE], STDOUT_FILENO );	
	
	/* kan stänga den ursprungliga FD */
	closeFileDesc(writePipeline[PIPE_WRITE]);	
	
	
	execlp( "sort", "sort", (char *) 0 );
	
	
	/* exec returnerar bara om något fel har uppstått
	   och om exec returnerar så är returvärdet alltid -1 */
	   
	perror( "Cannot exec sort" );
	exit( errno );
}

/**
 * Körs bara av childprocessen. Kallar på en pager som ska ta namnet av den temporära filen
 * som parameter.
 */
void handlePager(int readPipeline[]) {
	/* ersätt stdin med duplicerad läs-änden på readPipen */
	duplicate( readPipeline[ PIPE_READ ], STDIN_FILENO );	
	
	/* kan stänga den ursprungliga pipen */
	closeFileDesc(readPipeline[PIPE_READ] );	

	/* -------------Undersök om det finns en PAGER--------- */
	char* pager = getenv("PAGER");
	if(NULL != pager) {
		fprintf( stderr,"Found PAGER: %s\n",pager);
		execlp( pager, pager, NULL );
		
		/* Något gick fel med pagern*/
		perror("Cannot exec pager.");
		/* Pröva less */
		fprintf( stderr,"Trying with less\n");
	}
	else {
		fprintf( stderr,"No environment variable for PAGER\n");	
	}
	
	execlp( "less", "less", NULL );
	
	/* Något gick fel med less, pröva med more */
	fprintf( stderr,"Trying with more\n");
	
	execlp( "more", "more", NULL );
	
	/* Något gick fel med more. Ingen pager fungerade: avsluta */
	perror("Cannot exec more.");		
	exit( errno );	
}

/**
 * Initierar en pipe och kollar efter errors
 */
void createPipe(int pipeline[]) {
	int return_value = pipe( pipeline );
	/* avsluta programmet om pipe() misslyckas */
	if( -1 == return_value ) 	{
		perror( "Cannot create pipe" ); exit( errno ); 
	}
}

/**
 * Vänta på ett barn
 */
void waitForChild() {
	int status;
	
	child_pid = wait( &status );
	if( -1 == child_pid ) { 
		perror( "wait() failed unexpectedly" ); exit( errno ); 	
	}
	
	/* child-processen har kört klart */
	if( WIFEXITED( status ) ) {
		int child_status = WEXITSTATUS( status );
		
		/* child fick problem */
		if( 0 != child_status ) {
			/* Grep returnerar med exit(1) om den inte hittar någon matchning. */
			if(child_pid == grep_pid && child_status == 1) {
				fprintf( stderr,"No match found\n");
				/* Kill pager */
				kill(pager_pid,9);
			}
			/* Grep returnerar med exit(2) om den får konstiga parametrar. */
			else if(child_pid == grep_pid && child_status == 2) {
				fprintf( stderr,"Bad input\n");	
				kill(pager_pid,9);
			}
			/* Broken pipe */
			else if(child_status == 13) {
				fprintf( stderr,"Pipe broke\n");	
				kill(pager_pid,9);
			}
			else
				fprintf( stderr,"Child (pid %ld) failed with exit code %d\n", (long int) child_pid, child_status );
			
		}
	}
	else {
		/* child-processen avbröts av signal */
		if( WIFSIGNALED( status ) )	{
			int child_signal = WTERMSIG( status );
			fprintf( stderr,"Child (pid %ld) was terminated by signal no. %d\n",(long int) child_pid, child_signal );
		}
	}
}



/**
 * Stänger en fd och kollar returnvärdet.
 */
void closeFileDesc(int fd) {
	int return_value = close( fd );
	if( -1 == return_value ) { 
		perror( "Cannot close pipe" ); exit( errno ); 		
	}
}

/**
 * Kallar på dup2 och kollar returnvärdet. Tar samma parametrar som dup2
 */
void duplicate(int from, int to) {
	int return_value = dup2( from, to );
	if( -1 == return_value ) { 
		perror( "Cannot dup" ); exit( 1 ); 
	}
}
