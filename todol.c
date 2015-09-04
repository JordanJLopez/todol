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

#define MAX_DATA 64
#define MAX_TASKS 20

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
	printf("%2d : %64s\n", entry->id, entry->task);
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

int main(int argc, char *argv[])
{
	if(argc < 3)
		die("USAGE: todol <listfile> <action> [action parameters]");

	char* filename = argv[1];
	char action = argv[2][0];

	struct Connection *conn = List_open(filename, action);

	switch(action){
		case 'c':
			List_create(conn);
			List_write(conn);
			break;
		case 'a':
			printf("Enter new task: \n");
			char instr[MAX_DATA];
			fgets(instr, MAX_DATA, stdin);
			//instr[MAX_DATA - 1] = '\0';
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
			printf("#%67s#\n","");
			List_list(conn);
			printf("#%67s#\n","");
			break;
		case 'g':
			if(argc < 4)
				die("Need an ID to get");
			List_get(conn, atoi(argv[3]));
		default:
			die("Invalid action, only: c=create, a=add, r=remove, l=list, g=get");
	}

	List_close(conn);
	return 0;
}
