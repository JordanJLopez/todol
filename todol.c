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

#include <sys/ioctl.h>

#define MAX_DATA 256
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

void Entry_printf(struct Entry *entry)
{
	printf(ANSI_FONT_BOLD "%2d:\n" ANSI_COLOR_RESET "%s\n", entry->id, entry->task);
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

void Todol_header(int columns)
{
	//
	//	Todol_header creates one of several header lines randomly for Todol,
	//

	//For MAXDATA = 64, header and footer length must be 71 + # + \n
	//r is a random number between 0 and 9

	char filler[columns];
	if(columns < 38)
	{
		printf(ANSI_BACKGROUND_CYAN "%s" ANSI_COLOR_RESET "\n",filler);
	}
	else
	{
		srand(time(NULL));
		int r = rand() % 10;
		printf("\n"ANSI_BACKGROUND_CYAN "%s" ANSI_COLOR_RESET "\n",filler);
		switch(r)
		{
			case 0: // "# todol: You got this!" + " #" length: 24
				char leftJustify[columns - 24] = "";
				printf(ENDHASH ANSI_CYAN_BOLD " todol: You got this!" ANSI_COLOR_RESET "%s" ENDHASH "\n",leftJustify);
				break;
			case 1: // "# todol: I believe in you!" + " #" length:28
				char leftJustify[columns - 28] = "";
				printf(ENDHASH ANSI_CYAN_BOLD" todol: I believe in you!" ANSI_COLOR_RESET "%s" ENDHASH "\n",leftJustify);
				break;
			case 2: // "# todol: Just do it! -Shia LaBeouf" + " #" length:36
				char leftJustify[columns - 36] = "";
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Just do it! -Shia LaBeouf" ANSI_COLOR_RESET "%s" ENDHASH "\n",leftJustify);
				break;
			case 3: // "# todol: Make it a todone-l" + " #" length:29
				char leftJustify[columns - 29] = "";
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Make it a todone-l" ANSI_COLOR_RESET "%s" ENDHASH "\n",leftJustify);
				break; 
			case 4: // "# todol: You're awesome." + " #" length:26
				char leftJustify[columns - 26] = "";
				printf(ENDHASH ANSI_CYAN_BOLD" todol: You're awesome." ANSI_COLOR_RESET "%s" ENDHASH "\n",leftJustify);
				break; 
			case 5: // "# todol: You make it look easy." + " #" length:33
				char leftJustify[columns - 33] = "";
				printf(ENDHASH ANSI_CYAN_BOLD" todol: You make it look easy." ANSI_COLOR_RESET "%s" ENDHASH "\n",leftJustify);
				break; 
			case 6: // "# todol: You're on fire today!" + " #" length:32
				char leftJustify[columns - 32] = "";
				printf(ENDHASH ANSI_CYAN_BOLD" todol: You're on fire today!" ANSI_COLOR_RESET "%s" ENDHASH "\n",leftJustify);
				break; 
			case 7: // "# todol: Don't give up!" + " #" length:25
				char leftJustify[columns - 25] = "";
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Don't give up!" ANSI_COLOR_RESET "%s" ENDHASH "\n",leftJustify);
				break; 
			case 8: // "# todol: Take it one step at a time." + " #" length:38
				char leftJustify[columns - 38] = "";
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Take it one step at a time." ANSI_COLOR_RESET "%s" ENDHASH "\n",leftJustify);
				break; 
			case 9: // "# todol: Show this list who's boss!" + " #" length:37
				char leftJustify[columns - 37] = "";
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Show this list who's boss!" ANSI_COLOR_RESET "%s" ENDHASH "\n",leftJustify);
				break; 
			default: // "# todol: Nothing is impossible!" + " #" length:33
				char leftJustify[columns - 33] = "";
				printf(ENDHASH ANSI_CYAN_BOLD" todol: Nothing is impossible!" ANSI_COLOR_RESET "%s" ENDHASH "\n",leftJustify); 
				//Ironically, should be impossible to see this.
				break;
		}
	}
	
	printf(ANSI_BACKGROUND_CYAN "%s" ANSI_COLOR_RESET "\n",filler);

}

void Todol_footer(int columns)
{
	if(columns > 48)
	{
		char rightJustify[columns - 48];
		char* footertext = "c=create, a=add, r=remove, l=list, g=get, p=pop";
		printf(ANSI_BACKGROUND_CYAN "%s%s" ANSI_COLOR_RESET "\n",rightJustify,footertext);

	}
	else
	{
		char filler[columns];
		printf(ANSI_BACKGROUND_CYAN "%s" ANSI_COLOR_RESET "\n", filler);
	}
}

void Todol_emptylist()
{
	printf(ENDPIPE ANSI_CYAN_BOLD "%23sYou finished everything!%23s" ANSI_COLOR_RESET ENDPIPE "\n","","");
}

void List_listf(struct Connection *conn)
{
	struct winsize sz;
	if(!ioctl(0, TIOCGWINSZ, &sz))
	{
		printf("Screen width: %i Screen height: %i\n",sz.ws_col,sz.ws_row);
		Todol_header(sz.ws_col);
		int i = 0;
		for(i = 0; i < MAX_TASKS; i++)
		{
			if(conn->ls->tasks[i].set)
				Entry_printf(&conn->ls->tasks[i]);
		}
		Todol_footer(sz.ws_col);
	}
	else
	{
		Todol_header(72)
		List_list(conn);
		Todol_footer(72);
	}
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
			List_listf(conn);
			Todol_header();
			if(!conn->ls->tasks[0].set)
				Todol_emptylist();
			else
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
