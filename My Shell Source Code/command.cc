
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Create available space for 5 arguments
	_numOfAvailableArguments = 5;
	_numOfArguments = 0;
	_arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
		// Double the available space
		_numOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numOfArguments + 1] = NULL;
	
	_numOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_isAmbiguous = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_isAmbiguous = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

extern "C" void disp(int sig) {
	if (Command::_currentCommand._numOfSimpleCommands == 0) {
		Command::_currentCommand.clear();
		printf("\n");
		Command::_currentCommand.prompt();
	}
	else {
		printf("\n");
	}
}

void killzombie(int sig) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int isBuiltinCommand(char * cmd)
{
	char * builtins[] = {
		(char *)("setenv"),
		(char *)("unsetenv"),
		(char *)("cd"),
		(char *)("jobs"),
		(char *)("fg"),
		(char *)("bg"),
		(char *)("source")
	};
	for (int i = 0; i < 7; i++) {
		if (strcmp(cmd, builtins[i]) == 0) {
			return i + 1;
		}
	}
	return 0;
}

void executeBuiltin(char * cmd, char * args[]) {
	if (strcmp(cmd, "setenv") == 0) {
		int result;
		result = setenv(args[1], args[2], 1);
		if (result < 0) {
			perror("setenv");
		}
	}
	else if (strcmp(cmd, "unsetenv") == 0) {
		int result;
		result = unsetenv(args[1]);
		if (result < 0) {
			perror("unsetenv");
		}
	}
	else if (strcmp(cmd, "cd") == 0) {
		if (args[1] == NULL) {
			chdir(getenv("HOME"));
		}
		else {
			int ret;
			ret = chdir(args[1]);
			if (ret < 0) {
				perror(args[1]);
			}
		}
	}
	else if (strcmp(cmd, "jobs") == 0) {
		printf("jobs\n");
	}
	else if (strcmp(cmd, "fg") == 0) {
		printf("fg\n");
	}
	else if (strcmp(cmd, "bg") == 0) {
		printf("bg\n");
	}
	else if (strcmp(cmd, "source") == 0) {
		printf("source\n");
	}
	else {
		//should never reach this statement
		printf("error\n");
	}
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
//	print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	
	if (_isAmbiguous) {
		printf("Ambiguous output redirect");
	}

	//store initial in and out
	int defaultin = dup(0);
	int defaultout = dup(1);
	int defaulterr = dup(2);

	int fdin;
	int fdout;
	int fderr;

	//set initial input
	//redirect input if there is a file
	if (_inFile) {
		fdin = open(_inFile, O_RDONLY);
	}
	//otherwise use default input
	else {
		fdin = dup(defaultin);
	}

	int ret;
	for (int i = 0; i < _numOfSimpleCommands; i++) {
		dup2(fdin, 0);
		close(fdin);
		//output to file or default out for last command
		if (i == _numOfSimpleCommands - 1) {
			if (_outFile) {
				if (_append) {
					fdout = open(_outFile, O_WRONLY | O_CREAT | O_APPEND, 0600);
				}
				else {
					fdout = open(_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
				}
			}
			else {
				fdout = dup(defaultout);
			}
			if (_errFile) {
				if (_append) {
					fderr = open(_errFile, O_WRONLY | O_CREAT | O_APPEND, 0600);
				}
				else { 
					fderr = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
				}
				//redirect error
				dup2(fderr, 2);
				close(fderr);
			}
		}
		//output that isn't the last command is to a pipe
		else {
			int fdpipe[2];
			pipe(fdpipe);
			fdout = fdpipe[1];
			fdin = fdpipe[0];
		}

		dup2(fdout, 1);
		close(fdout);

		if (isBuiltinCommand(_simpleCommands[i]->_arguments[0])) {
			executeBuiltin(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
		}
		else {
			ret = fork();
			//if in the child process
			if (ret == 0) {
				//load program into current process
				execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
				perror("execvp");
				_exit(1);
			}
			else if (ret < 0) {
				perror("fork");
				return;
			}
		}
		//parent shell continues
	}

	//restore in/out/error defaults
	dup2(defaultin, 0);
	dup2(defaultout, 1);
	close(defaultin);
	close(defaultout);
	if(_errFile) {
		dup2(defaulterr, 2);
	}
	close(defaulterr);

	if (!_background) {
		//wait for last process
		//perror("background");
		waitpid(ret, NULL, 0);
	}

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	if (isatty(0)){
		printf("myshell>");
		fflush(stdout);
	}
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

main()
{
	Command::_currentCommand.prompt();
	
	struct sigaction sa;
	sa.sa_handler = disp;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	struct sigaction saz;
	saz.sa_handler = killzombie;
	sigemptyset(&saz.sa_mask);
	saz.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &saz, NULL)) {
		perror("sigaction zombie");
		exit(-1);
	}

	if (sigaction(SIGINT, &sa, NULL)) {
		perror("sigaction");
		exit(2);
	}

	yyparse();
}

