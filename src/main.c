#include <stdio.h>
#include "database_connector.h"
#include "CMD.h"

int main() {
    DbManager db;
    if (!db_init(&db, "172.17.0.2", "root", "12332145", "linux")) return 1;

    db_execute(&db, "INSERT INTO test(name) VALUES('John')");

    db_close(&db);
    return 0;
}