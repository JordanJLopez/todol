//
//	todol: a simple, text-based to-do list
//
//	USAGE: todol <listfile> <action> [action parameters]
//
//	Actions: c=create, a=add, d=del, l=list, g=get, ch=change
//

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define MAX_DATA 64
#define MAX_TASKS 20

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_CYAN_BOLD	   "\033[1;36m"

#define ANSI_FONT_BOLD	   "\x1b[1m"
#define ANSI_BACKGROUND_CYAN "\033[7;36m"

#define ENDHASH ANSI_BACKGROUND_CYAN " " ANSI_COLOR_RESET
#define ENDPIPE ANSI_BACKGROUND_CYAN " " ANSI_COLOR_RESET

struct Entry {
	int id;
	int set;
	char task[MAX_DATA];
};

struct List {
	struct Entry tasks[MAX_TASKS];
};

struct Connection {
	FILE *file;
	struct List *ls;
};

void die(const char *message)
{
	if(errno) {
		perror(message);
	} else {
		printf("ERROR: %s\n", message);
	}


	exit(1);
}

void Entry_print(struct Entry *entry)
{
	printf(ENDPIPE "%2d : %64s " ENDPIPE "\n", entry->id, entry->task);
}

void List_load(struct Connection *conn)
{
	int rc = fread(conn->ls, sizeof(struct List), 1, conn->file);
	if(rc != 1)
		die("Failed to load list.");
}

struct Connection *List_open(const char *filename, char mode)
{
	struct Connection *conn = malloc(sizeof(struct Connection));
	if(!conn)
		die("Memory error");

	conn->ls = malloc(sizeof(struct List));
	if(!conn->ls)
		die("Memory error");

	if(mode == 'c') {
		conn->file = fopen(filename, "w");
	} else {
		conn->file = fopen(filename, "r+");

		if(conn->file) {
			List_load(conn);
		}
	}

	if(!conn->file)
		die("Failed to open the file");

	return conn;
}

void List_close(struct Connection *conn)
{
	if(conn) {
		if(conn->file)
			fclose(conn->file);
		if(conn->ls)
			free(conn->ls);
		free(conn);
	}
}

void List_write(struct Connection *conn)
{
	rewind(conn->file);

	int rc = fwrite(conn->ls, sizeof(struct List), 1, conn->file);
	if(rc != 1)
		die("Failed to write list.");

	rc = fflush(conn->file);
	if(rc == -1)
		die("Cannot flush database.");
}

void List_create(struct Connection *conn)
{
	int i = 0;

	for(i = 0; i < MAX_TASKS; i++)
	{
		struct Entry entry = {.id=i, .set=0};
		conn->ls->tasks[i] = entry;
	}
}

int List_add(struct Connection *conn, char* task)
{
	int i = 0;
	for(i = 0; i < MAX_TASKS; i++)
		if(!conn->ls->tasks[i].set) {
			strncpy(conn->ls->tasks[i].task, task, MAX_DATA);
			conn->ls->tasks[i].task[MAX_DATA - 1] = '\0';
			conn->ls->tasks[i].set = 1;
			return i;
		}

	die("List full: please remove an entry");
	return -1;
}


int List_remove(struct Connection *conn, int id)
{
	//Checks if ID is within valid range
	if(id < 0 || id >= MAX_TASKS)
		die("ID out of range");

	//Checks if ID is already empty
	if(!conn->ls->tasks[id].set)
		return 0;

	struct Entry empty = {.id = id, .set = 0};


	//Checks if ID is the greatest possible ID
	if(id == MAX_TASKS -1)
	{
		conn->ls->tasks[id] = empty;
		return 0;
	}

	//Checks if ID + 1 is empty, and if so, just remove ID
	if(!conn->ls->tasks[id+1].set)
	{
		conn->ls->tasks[id] = empty;
		return 0;
	}

	//Else, move data from ID + 1 to ID, then call to remove ID + 1;
	conn->ls->tasks[id] = conn->ls->tasks[id+1];
	return List_remove(conn, id+1);
}

void List_list(struct Connection *conn)
{
	int i = 0;

	for(i = 0; i<MAX_TASKS; i++) {
		if(conn->ls->tasks[i].set)
			Entry_print(&conn->ls->tasks[i]);
	}
}

void List_get(struct Connection *conn, int id)
{
	if(id<0 || id>=MAX_TASKS)
		die("ID out of range");
	if(conn->ls->tasks[id].set)
		Entry_print(&conn->ls->tasks[id]);
	else
		die("No task with given ID");
}

void Todol_header()
{
	//
	//	Todol_header creates one of several header lines randomly for Todol,
	//

	//For MAXDATA = 64, header and footer length must be 71 + # + \n
	//r is a random number between 0 and 9
	srand(time(NULL));
	int r = rand() % 10;
	printf("\n"ENDHASH ANSI_BACKGROUND_CYAN "%70s" ENDHASH "\n","");
	switch(r)
	{
		case 0: // "# todol: You got this!" length: 22
			printf(ENDHASH ANSI_CYAN_BOLD " todol: You got this!" ANSI_COLOR_RESET "%49s" ENDHASH "\n","");
			break;
		case 1: // "# todol: I believe in you!" length:26
			printf(ENDHASH ANSI_CYAN_BOLD" todol: I believe in you!" ANSI_COLOR_RESET "%45s" ENDHASH "\n","");
			break;
		case 2: // "# todol: Just do it! -Shia LaBeouf" length:34
			printf(ENDHASH ANSI_CYAN_BOLD" todol: Just do it! -Shia LaBeouf" ANSI_COLOR_RESET "%37s" ENDHASH "\n","");
			break;
		case 3: // "# todol: Make it a todone-l" length:27
			printf(ENDHASH ANSI_CYAN_BOLD" todol: Make it a todone-l" ANSI_COLOR_RESET "%44s" ENDHASH "\n","");
			break; 
		case 4: // "# todol: You're awesome." length:24
			printf(ENDHASH ANSI_CYAN_BOLD" todol: You're awesome." ANSI_COLOR_RESET "%47s" ENDHASH "\n","");
			break; 
		case 5: // "# todol: You make it look easy." length:31
			printf(ENDHASH ANSI_CYAN_BOLD" todol: You make it look easy." ANSI_COLOR_RESET "%40s" ENDHASH "\n","");
			break; 
		case 6: // "# todol: You're on fire today!" length:30
			printf(ENDHASH ANSI_CYAN_BOLD" todol: You're on fire today!" ANSI_COLOR_RESET "%41s" ENDHASH "\n","");
			break; 
		case 7: // "# todol: Don't give up!" length:23
			printf(ENDHASH ANSI_CYAN_BOLD" todol: Don't give up!" ANSI_COLOR_RESET "%48s" ENDHASH "\n","");
			break; 
		case 8: // "# todol: Take it one step at a time." length:36
			printf(ENDHASH ANSI_CYAN_BOLD" todol: Take it one step at a time." ANSI_COLOR_RESET "%35s" ENDHASH "\n","");
			break; 
		case 9: // "# todol: Show this list who's boss!" length:35
			printf(ENDHASH ANSI_CYAN_BOLD" todol: Show this list who's boss!" ANSI_COLOR_RESET "%36s" ENDHASH "\n","");
			break; 
		default: // "# todol: Nothing is impossible!" length:31
			printf(ENDHASH ANSI_CYAN_BOLD" todol: Nothing is impossible!" ANSI_COLOR_RESET "%40s" ENDHASH "\n",""); 
			//Ironically, should be impossible to see this.
			break;
	}
	printf(ENDHASH ANSI_BACKGROUND_CYAN "%70s" ENDHASH "\n","");
	printf(ENDHASH "%70s" ENDHASH "\n","");

}

void Todol_footer()
{
	printf(ENDHASH "%70s" ENDHASH "\n","");
	printf(ENDHASH ANSI_BACKGROUND_CYAN "%70s" ENDHASH "\n","");
}

int main(int argc, char *argv[])
{
	if(argc < 3)
		die("USAGE: todol <listfile> <action> [action parameters]");

	char* filename = argv[1];
	char action = argv[2][0];

	struct Connection *conn = List_open(filename, action);
	int i;
	switch(action){
		case 'c':
			List_create(conn);
			List_write(conn);
			break;
		case 'a':
			printf("Enter new task: \n");
			char instr[MAX_DATA];
			fgets(instr, MAX_DATA-1, stdin);

			//Scrubs input of newline character
			char *pos;
			if((pos = strchr(instr, '\n')) != NULL)
				*pos = '\0';
			instr[MAX_DATA - 1] = '\0';
			printf("New ID: %d\n", List_add(conn, instr));
			List_write(conn);
			break;
		case 'r':
			if(argc < 4)
				die("Need an ID to remove");
			List_remove(conn, atoi(argv[3]));
			List_write(conn);
			break;
		case 'l':
			Todol_header();
			List_list(conn);
			Todol_footer();
			break;
		case 'g':
			if(argc < 4)
				die("Need an ID to get");
			List_get(conn, atoi(argv[3]));
		case 'p':
			for(i = MAX_TASKS -1; i>=0; i--)
				if(conn->ls->tasks[i].set)
				{
					printf("Removing ID: %d\n", i); 
					List_remove(conn, i);
					List_write(conn);
					break;
				}
			break;
		default:
			die("Invalid action, only: c=create, a=add, r=remove, l=list, g=get, p=pop");
	}

	List_close(conn);
	return 0;
}
