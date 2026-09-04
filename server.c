#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<unistd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t')

// Given structure for the Inode	
typedef struct {
        uint32_t inode;
        uint32_t parentInode;
        char type;
        char name[32];
} Inode;

Inode inodeList[1024]; // an array of inodes where each inode represents a file or a directory
size_t inodeCount = 0; // a count of inodes in the list, initialized to zero
uint32_t currentInode= 0; // a variable to hold and keep track of the current inode
uint32_t prevInode = 0;

void loadInodeList() {

	FILE *fp = fopen("inodes_list", "rb");
	if(fp == NULL) {
		
		fp = fopen("inodes_list", "wb");
    if (fp == NULL) {
        perror("Could not create inodes list file");
        exit(1);
    }

    memset(inodeList, 0, sizeof(inodeList));

    inodeList[0].inode = 0;
    inodeList[0].parentInode = 0;
    inodeList[0].type = 'd';
    strncpy(inodeList[0].name, "demo", 31);
    inodeList[0].name[31] = '\0';

    for (int i = 1; i < 1024; i++) {
        inodeList[i].type = 'u';
        inodeList[i].name[0] = '\0';
        inodeList[i].inode = (uint32_t)i;
        inodeList[i].parentInode = 0;
    }

    size_t written = fwrite(inodeList, sizeof(Inode), 1024, fp);
    if (written != 1024) {
        perror("Could not write initial inode table");
        fclose(fp);
        exit(1);
    }

    fclose(fp);
    inodeCount = 1;
    currentInode = 0;
    return;
	}
	
	// read the inodes list file to get count 
	size_t count = fread(inodeList, sizeof(Inode), 1024, fp);
	if(count == 0) {
		printf("Could not read inodes\n");
		fclose(fp);
		exit(1);
	}
	
	inodeCount = count;
	fclose(fp);
}

void saveInodeList() {

	FILE *fp = fopen("inodes_list", "wb");
	if(fp == NULL) {
		printf("Could not open inodes list file\n");
		exit(1);
	} 

	// write into inodes list file from the indoes array
	size_t inodes_written = fwrite(inodeList, sizeof(Inode), inodeCount, fp);
	if(inodes_written != inodeCount) {
		printf("Error writing to inodes list file\n");

		fclose(fp);
		exit(1);
	}
	
	fclose(fp);
}

int rm(const char *name) {

	if (name == NULL || name[0] == '\0') {
        	printf("rm: missing operand\n");
        	return -1;
    	}

    	int match = -1;

    	for (int i = 0; i < inodeCount; i++) {
        	if (inodeList[i].parentInode == currentInode &&
            	strcmp(inodeList[i].name, name) == 0) {
         		match = i;
         		break;
        	}
    	}

    	if (match == -1) {
        	printf("rm: cannot remove '%s': No such file\n", name);
    	    	return -1;
    	}

    	if (inodeList[match].type == 'd') {
        	printf("rm: cannot remove '%s': Is a directory\n", name);
        	return -1;
    	}

    	if (inodeList[match].type != 'f') {
        	printf("rm: cannot remove '%s': Not a file\n", name);
       		return -1;
    	}

    	inodeList[match].type = 'u';
    	inodeList[match].name[0] = '\0';
    	inodeList[match].parentInode = 0;
        return 0;		
}

void changeDirectory(const char *name) {

	int foundIt = 0;

	if (name == NULL || name[0] == '\0') {
           currentInode = 0;
                return;
        }

	if (strcmp(name, "..") == 0) {
        	if (currentInode != 0) {   // prevent going above root
            	currentInode = inodeList[currentInode].parentInode;
        	}
	        return;
    	}

	for(int i = 0; i < inodeCount; i++) {
		if((strcmp(inodeList[i].name, name) == 0) && inodeList[i].type == 'd' && inodeList[i].parentInode == currentInode) {
			currentInode = i;
			foundIt = 1;
			break;
		} 
	}

	
	if(foundIt == 0){
		printf("Directory does not exist\n");
	}	
}

void listContents() {

	for(int i = 0; i < inodeCount; i++) {
		if (inodeList[i].type == 'u') continue;
		if(inodeList[i].parentInode == currentInode) {
			printf("inode: %u, type: %c, name: %s\n", inodeList[i].inode, inodeList[i].type, inodeList[i].name);
		}
	}
}

void createDirectory(const char *name) {

	for(int i = 0; i < inodeCount; i++) {
		if(inodeList[i].parentInode == currentInode && strcmp(inodeList[i].name, name) == 0) {
			printf("Directory already exists\n");
			return;	
		}		
	}

	if(inodeCount >= 1024) {
		printf("Not enough storage\n");
		return;
	}
	
	// create the new Inode and set its parameters
	Inode *newInode = &inodeList[inodeCount];
	newInode->inode = inodeCount;
	newInode->parentInode = currentInode;
	newInode->type = 'd';
	strncpy(newInode->name, name, sizeof(newInode->name) - 1);
	newInode->name[sizeof(newInode->name) - 1] = '\0';
	inodeCount += 1;
	
	// create the file
	char filename[32];
	sprintf(filename, "%u", newInode->inode);
	
	FILE *fp;
	fp = fopen(filename, "w");
	if(fp == NULL) {
		printf("ERROR creating or opening new directory file\n");
		return;
	}

	// write its . and .. inode values
	char buffer[64];
	int len;
	len = sprintf(buffer, "%u .\n", newInode->inode);
	fwrite(buffer, 1, len, fp);
	len = sprintf(buffer, "%u ..\n", inodeList[currentInode].inode);
	fwrite(buffer, 1, len, fp);	
	
	fclose(fp);		
}

void createFile(const char *name) {

	for(int i = 0; i < inodeCount; i ++) {
		if(strcmp(inodeList[i].name, name) == 0 && (inodeList[i].parentInode == currentInode) && inodeList[i].type == 'f') {
			printf("File already exists\n");
			return;	
		}	
	}
	
	if(inodeCount >= 1024) {
                printf("Not enough storage\n");
                return;
        }
	
	// creathe file inode and set its parameters
	Inode *newInode = &inodeList[inodeCount];
	newInode->inode = inodeCount;
	newInode->parentInode = currentInode;
	newInode->type = 'f';
	strncpy(newInode->name, name, sizeof(newInode->name) - 1);
	newInode->name[sizeof(newInode->name) - 1] = '\0';
	inodeCount += 1;
	
	// create the file
	char filename[32];

	sprintf(filename, "%u", newInode->inode);

	FILE *fp;
	fp = fopen(filename, "w");
	if(fp == NULL) {
		printf("ERROR creating or opening new file\n");
		return;
	}

	// write into the file the file name	
	char buffer[64];
	int len;
	len = sprintf(buffer, "%s\n", newInode->name);
	fwrite(buffer, 1, len, fp);
	
	fclose(fp); 	
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		printf("Incorrect number of arguments\n");
		return 1;
	}

	// change into initial fs directory
	if(chdir(argv[1]) != 0) {
		perror("Error changing into initial directory\n");
		return 1;
	}

	// Load the inode list and list inode 0 contents
	loadInodeList();
	

	// need to check if we are starting at inode 0
	int found = 0;
	for(int i = 0; i < inodeCount; i++) {
		if(inodeList[i].inode == 0) {
			if(inodeList[i].type != 'd') {
				printf("Error: inode 0 is not a directory");
				exit(1);
			}
			found = 1;
			break;
		}
	}
	
	if(found != 1) {
		printf("Error: unable to find inode 0");
		exit(1);	
	}	

	// to read user input and store it in buffer 
	size_t buffer_size = 0;
	char *buffer = NULL;
	ssize_t characters = 0;
	int argument_count = 0;
	printf("> ");

	while((characters = getline(&buffer, &buffer_size, stdin)) != -1) {

		// remove the trailing '\n' in the user input so strcmp can work properly
		if(characters > 0 && buffer[characters - 1] == '\n') {
			buffer[characters - 1] = '\0';
			characters -= 1;
		}

		// empty input check
		if(characters == 0) {
			printf("> ");
			continue;
		}
		
		// initialize pointers for command and argument so we don't loose heap reference
		char *temp1 = buffer;
		char *scan = temp1;
		char *command = NULL;
		char *argument = NULL;
		size_t count = 0;
		argument_count = 0;
		
		// find the length of the command and stop at a space or end of line
		while(*scan != '\0' && IS_SPACE(*scan)) {
			scan += 1;	
		}
		
		count = 0;
		char *cmd_start = scan; // pointer for where the command (first argument) starts

		while(*scan != '\0' && !IS_SPACE(*scan)) {
			scan += 1;
			count += 1;
		}

		if(count == 0) {
			printf("Invalid input\n");
			printf("> ");
			continue;
		}

		command = malloc(count + 1); // request heap spaace for the command
		if(command == NULL) { 
			perror("Error using malloc\n");
			break;
		}

		strncpy(command, cmd_start, count); // copy first count characters from the command start to the command heap space
		command[count] = '\0'; // add end of line delimiter to the command (first argument)

		// iterate until there is no more spaces or end of line
		while(*scan != '\0' && IS_SPACE(*scan)) {
			scan += 1;
		}

		// ls/exit scenario, also set 2nd argument (argument) to NULL	
		if(*scan == '\0') {
			argument_count = 1;
			argument = NULL;
		} else { // two arguments case	
			argument_count = 2;
			char *arg_start = scan; // pointer for where the argument (second argument) starts
			count = 0; // reset count variable

			// iterate until end of line or a space appears
			while(*scan != '\0' && !IS_SPACE(*scan)) {
				scan += 1;
				count += 1;
			}

			// requesting heap space for the 2ns argument (argument)
			argument = malloc(count + 1);
			if(argument == NULL) {
				perror("Error using malloc\n");
				free(command);
				break;
			}
		
			strncpy(argument, arg_start, count); // copy the first count characters from the argument start to the argument heap space
			argument[count] = '\0'; // add delimiter to the argument (second argument)

			while(*scan != '\0' && IS_SPACE(*scan)) {
				scan += 1;
			}
			if(*scan != '\0') {
				printf("Error too many arguments\n");
				free(command);
				free(argument);
				printf("> ");
				continue;
			}
		}

		// exit condition
		if(argument_count == 1 && strcmp(command, "exit") == 0) {
			free(command);
			free(argument);
			saveInodeList();
			break;
		}
		
		// ls condition
		if(argument_count == 1 && strcmp(command, "ls") == 0) {
			listContents();
			free(command);
			free(argument);
			printf("> ");
			continue;
		}
		
		// cd with no argument -> go to root
	 	if (argument_count == 1 && strcmp(command, "cd") == 0) {
			changeDirectory(NULL);
		        free(command);
		        free(argument);          
		        printf("> ");
		        continue;
		}

		// cd/mkdir/touch conditions
		// these are special becuase they have text after the command
		if(argument_count == 2) {
			char name32[33];
			strncpy(name32, argument, 32);
			name32[32] = '\0';
		
			if(strcmp(command, "cd") == 0) {
				changeDirectory(name32);
				free(command);
				free(argument);
				printf("> ");
				continue;
			} else if(strcmp(command, "mkdir") == 0) {
				createDirectory(name32);
				free(command);
				free(argument);
				printf("> ");
				continue;
			} else if(strcmp(command, "touch") == 0) {
				createFile(name32);
				free(command);
				free(argument);
				printf("> ");
				continue;
			} else if(strcmp(command, "rm") == 0) {
 				pid_t pid = fork();
    				if (pid < 0) {
        			perror("fork");
        			free(command);
        			free(argument);
        			printf("> ");
        			continue;
    			}

    				if (pid == 0) {
        				int rc = rm(name32);
        				if (rc == 0) saveInodeList();
        				_exit(rc == 0 ? 0 : 1);
    				} else {
        				int status = 0;
        				waitpid(pid, &status, 0);
        				loadInodeList();
        				free(command);
        				free(argument);
        				printf("> ");
        				continue;
    				}
			}
		}
		printf("Error: invalid command\n");
		free(command);
		free(argument);
		printf("> ");
	}

	saveInodeList();
	free(buffer);
	return 0;			
}

