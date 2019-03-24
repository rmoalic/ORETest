#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <mysql/mysql.h>
#include "ore.h"

#define N 40
struct encrypt {
    ore_params params;
    ore_secret_key sk;
};

void show_mysql_error(MYSQL *conn)
{
  printf("Error(%d) [%s] \"%s\"\n", mysql_errno(conn),
                                  mysql_sqlstate(conn),
                                  mysql_error(conn));
  exit(EXIT_FAILURE);
}


static void show_stmt_error(MYSQL_STMT *stmt)
{
  printf("Error(%d) [%s] \"%s\"\n", mysql_stmt_errno(stmt),
                                    mysql_stmt_sqlstate(stmt),
                                    mysql_stmt_error(stmt));
  exit(EXIT_FAILURE);
}

MYSQL* init_mysql() {
    MYSQL* conn;
    char *server = "localhost";
    char *user = "root";
    char *password = "";
    char *database = "employer";
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0))
        show_mysql_error(conn);
    return conn;
}

void reinit(MYSQL* conn) {
    if (mysql_query(conn, "DROP TABLE IF EXISTS personnes"))
        show_mysql_error(conn);

    if (mysql_query(conn, "CREATE TABLE personnes (id serial, name varchar(30), salary blob)"))
        show_mysql_error(conn);
}

void add_employee(MYSQL* conn, struct encrypt* ec, char* name, int salary) {
    ore_ciphertext ct;
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[2];
    char ind[1]= {STMT_INDICATOR_NTS};
 
    stmt= mysql_stmt_init(conn);
    if (mysql_stmt_prepare(stmt, "INSERT INTO personnes(name, salary) VALUES (?,?)", -1))
        show_stmt_error(stmt);

    init_ore_ciphertext(ct, ec->params);
    ore_encrypt_ui(ct, ec->sk, salary);

    memset(bind, 0, sizeof(MYSQL_BIND) * 2);
    unsigned long payload_size = (unsigned long)N * sizeof(uint64_t);

    bind[0].buffer= &name;
    bind[0].buffer_type= MYSQL_TYPE_STRING;
    bind[0].u.indicator= ind;
    bind[1].buffer_type= MYSQL_TYPE_BLOB;
    bind[1].buffer= &(ct[0].buf);
    bind[1].buffer_length = payload_size;
    bind[1].u.indicator= ind;

    int size = 1;
    mysql_stmt_attr_set(stmt, STMT_ATTR_ARRAY_SIZE, &size);
    mysql_stmt_bind_param(stmt, bind);
    if (mysql_stmt_execute(stmt))
        show_stmt_error(stmt);

    mysql_stmt_close(stmt);
}

void show_data(MYSQL* conn, bool salary_sorted, struct encrypt* ec) {
    if (salary_sorted)
        mysql_query(conn, "select * from personnes order by salary desc");
    else
        mysql_query(conn, "select * from personnes order by id asc");
        
    MYSQL_RES* res =  mysql_store_result(conn);
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        printf("%s\t%s\t", row[0], row[1]);

        if (ec != NULL) {
            
        } else {
            for (int i=0; i<N; i++)
                printf("%.02X ", row[2][i] & 0xFF);
        }

        puts("");
    }

    mysql_free_result(res);
}

void help() {
    puts("Commands:");
    puts("\tADD <name> <salary>\tAdd a new employee to the database");
    puts("\tLIST\tlist all employees (by id)");
    puts("\tSLIST\tlist all employees (by salary)");
    puts("\tRESET\tdelete and recreate database");
    puts("\tHELP\tprint this help message");
    puts("\tQUIT\texit the programm");
}

int main(int argc, char* argv[]) {
    MYSQL *conn = init_mysql();

    struct encrypt ec;
    ore_ciphertext ct;

    init_ore_params(ec.params, sizeof(uint64_t), N);
    init_ore_ciphertext(ct, ec.params);
    ore_setup(ec.sk, ec.params);
    
    help();
	char *p;
	while ((p = readline("prototype$ ")) != NULL) {
        char* token = strtok(p, " ");
        if (token == NULL)
            continue;
        if (strcmp(token, "ADD") == 0) {
            char* name = strtok(NULL, " ");
            char* salary_str = strtok(NULL, " ");
            if (name == NULL || salary_str == NULL) {
                puts("incorect parameters");
                continue;
            }
            int salary = atoi(salary_str);
            
            printf("Adding %s with %d\n", name, salary);
            add_employee(conn, &ec, name, salary);
        } else if (strcmp(token, "LIST") == 0) {
            show_data(conn, false, NULL);
        } else if (strcmp(token, "SLIST") == 0) {
            show_data(conn, true, NULL);
        } else if (strcmp(token, "HELP") == 0) {
            help();
        } else if (strcmp(token, "RESET") == 0) {
            reinit(conn);
        } else if (strcmp(token, "QUIT") == 0) {
            return EXIT_SUCCESS;
        }
	}

    return EXIT_SUCCESS;
}
