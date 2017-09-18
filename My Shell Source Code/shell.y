
/*
 *
 * shell.y: parser for shell
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE

%token GREATGREAT PIPE AMPERSAND GREATGREATAMPERSAND GREATAMPERSAND LESS

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <string.h>
#include <stdio.h>
#include <regex.h>
#include <dirent.h>
#include <assert.h>
#include "command.h"
int arrayInitialized = 0;
int maxEntries;
int nEntries;
char ** array;
void yyerror(const char * s);
char * wildcardToRegExpression(char * arg);
void expandWildcardsIfNecessary(char * arg);
void expandWildcard(char * prefix, char * suffix);
void addToArray(char * arg);
static int compare(const void * a, const void * b);
void sort(char *arr[], int n);
int yylex();

%}

%%

goal:	
	command_list
	;

command_list:
	command_line
	| command_list command_line
	; /* command loop */

command_line:
	pipe_list io_modifier_list background_opt NEWLINE {
		/*printf("   Yacc: Execute command line\n");*/
		Command::_currentCommand.execute();
	}
	| NEWLINE /* accept empty cmd line */
	| error NEWLINE { yyerrok; } /* error recovery */
	;

pipe_list:
	pipe_list PIPE command_and_args
	| command_and_args
	;

command_and_args:
	command argument_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

argument_list:
	argument_list argument
	| /* can be empty */
	;

argument:
	WORD {
               /*printf("   Yacc: insert argument \"%s\"\n", $1);*/

		//initialize array if it hasn't been initialized
		if (!arrayInitialized) {
			maxEntries = 20;
			nEntries = 0;
			array = (char**) malloc(maxEntries*sizeof(char*));
			arrayInitialized = 1;
		}

		expandWildcard("", $1);

		//sort the entries
		sort(array, nEntries);
		//add arguments
		for (int i = 0; i < nEntries; i++) {
			Command::_currentSimpleCommand->insertArgument(array[i]);
		}
		free(array);
		arrayInitialized = 0;
	}
	;

command:
	WORD {
               /*printf("   Yacc: insert command \"%s\"\n", $1);*/
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

io_modifier_list:
	io_modifier_list io_modifier
	| /* empty */
	;

io_modifier:
	GREAT WORD {
		/*printf("   Yacc: insert output \"%s\"\n", $2);*/
		if (Command::_currentCommand._outFile != 0) {
			Command::_currentCommand._isAmbiguous = 1;
		}
		Command::_currentCommand._outFile = $2;
	}
	| LESS WORD {
		/*printf("   Yacc: insert input \"%s\"\n", $2);*/
		Command::_currentCommand._inFile = $2;
	}
	| GREATAMPERSAND WORD {
		/*printf("   Yacc: insert output and error \"%s\"\n", $2);*/
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = strdup($2);
	}
	| GREATGREAT WORD {
		/*printf("   Yacc: insert output and append \"%s\"\n", $2);*/
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._append = 1;
	}
	| GREATGREATAMPERSAND WORD {
		/*printf("   Yacc: insert output error and append \"%s\"\n", $2);*/
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = strdup($2);
		Command::_currentCommand._append = 1;
	}
	;

background_opt:
	AMPERSAND {
		/*printf("   Yacc: run in background\n");*/
		Command::_currentCommand._background = 1;
	}
	| /* empty */
	;
%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

static int compare(const void * a, const void * b) {
	return strcmp(*(const char **) a, *(const char **) b);
}

void sort(char *arr[], int n) {
	qsort(arr, n, sizeof(const char *), compare);
}

char * wildcardToRegExpression(char * arg) {
	//convert wildcard to reg expression
	char * reg = (char *) malloc(2 * strlen(arg) + 10);
	char * a = arg;
	char * r = reg;
	*r = '^';
	r++;
	while (*a) {
		//convert "*" to ".*"
		if (*a == '*') {
			*r = '.';
			r++;
			*r = '*';
			r++;
		}
		//convert "?" to "."
		else if (*a == '?') {
			*r = '.';
			r++;
		}
		//convert "." to "\."
		else if (*a == '.') {
			*r = '\\';
			r++;
			*r = '.';
			r++;
		}
		//all other characters stay the same
		else {
			*r = *a;
			r++;
		}
		a++;
	}
	*r = '$';
	r++;
	*r = 0;
	return reg;
}

void expandWildcard(char * prefix, char * suffix) {
	//if suffix is empty, put prefix in argument
	if (suffix[0] == 0) {
		//eliminate double slashes
		if (prefix[0] == '/' && prefix[1] == '/') {
			prefix++;
		}

		//add argument to array
		addToArray(strdup(prefix));
		return;
	}
	//flag to determine whether suffix begins with "/"
	int suffixFirstCharIsSlash;
	//obtain the next component in the suffix
	//also advance suffix
	char * s = strchr(suffix, '/');
	char component[1024];
	//if s points to "/" location in string, copy up to and not including that first "/"
	if (s != NULL) {
		int numOfCharsToCopy = s - suffix;
		strncpy(component, suffix, numOfCharsToCopy);
		suffix = s + 1;
		//terminate component with 0
		component[numOfCharsToCopy] = '\0';
		if (numOfCharsToCopy == 0) {
			suffixFirstCharIsSlash = 1;
		}
		else {
			suffixFirstCharIsSlash = 0;
		}
	}
	//else this is the last part of the path, copy whole thing
	else {
		strcpy(component, suffix);
		suffix = suffix + strlen(suffix);
		suffixFirstCharIsSlash = 0;
	}

	//now expand the component
	char newPrefix[1024];
	//if component has no wildcards
	if (strchr(component, '*') == NULL && strchr(component, '?') == NULL) {
		if (suffixFirstCharIsSlash || prefix[0] != 0) {
			sprintf(newPrefix, "%s/%s", prefix, component);
		}
		else {
			sprintf(newPrefix, "%s%s", prefix, component);
		}
		expandWildcard(newPrefix, suffix);
		return;
	}
	//otherwise component has wildcards
	//convert component to regular expression
	char * reg;
	reg = wildcardToRegExpression(component);

	//compile regular expression
	regex_t preg;
	int rc;
	rc = regcomp(&preg, reg, REG_EXTENDED | REG_NOSUB);
	if (rc != 0) {
		perror("regcomp");
		return;
	}
	free(reg);

	char * dir;
	//if prefix is empty then list current directory
	if (prefix[0] == 0) {
		dir = ".";
	}
	else {
		dir = prefix;
	}
	DIR * d = opendir(dir);
	struct dirent * ent;
	if (d == NULL) {
		return;
	}

	//check what entries match
	while ((ent = readdir(d)) != NULL) {
		rc = regexec(&preg, ent->d_name, (size_t) 0, NULL, 0);
		//if name matches add entry to prefix and recursively call
		if (rc == 0) {
			//if entry is prefixed with ".", only add it if hidden files were specified
			if (ent->d_name[0] == '.') {
				if (component[0] == '.') {
					if (suffixFirstCharIsSlash || prefix[0] != 0) {
						sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
					}
					else {
						sprintf(newPrefix, "%s%s", prefix, ent->d_name);
					}
					expandWildcard(newPrefix, suffix);
				}
			}
			else {
				if (suffixFirstCharIsSlash || prefix[0] != 0) {
					sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
				}
				else {
					sprintf(newPrefix, "%s%s", prefix, ent->d_name);
				}
				expandWildcard(newPrefix, suffix);
			}
		}
	}
	regfree(&preg);
	closedir(d);
}

void addToArray(char * arg) {
	//resize the array if the maximum number of entries limit is reached
	if (nEntries == maxEntries) {
		maxEntries *= 2;
		array = (char**) realloc(array, maxEntries * sizeof(char*));
		assert(array != NULL);
	}
	array[nEntries] = arg;
	nEntries++;
}

#if 0
main()
{
	yyparse();
}
#endif
