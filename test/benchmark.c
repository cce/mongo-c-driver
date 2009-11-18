/* test.c */

#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#define ASSERT(x) \
    do{ \
        if(!(x)){ \
            printf("failed assert (%d): %s\n", __LINE__,  #x); \
            exit(1); \
        }\
    }while(0)

/* supports preprocessor concatenation */
#define DB "benchmarks"

#define PER_TRIAL 5000
#define BATCH_SIZE  100

static mongo_connection conn;

static void make_small(bson * out, int i){
    bson_buffer bb;
    bson_buffer_init(&bb);
    bson_append_new_oid(&bb, "_id");
    bson_append_int(&bb, "x", i);
    bson_from_buffer(out, &bb);
}

static void make_medium(bson * out, int i){
    bson_buffer bb;
    bson_buffer_init(&bb);
    bson_append_new_oid(&bb, "_id");
    bson_append_int(&bb, "x", i);
    bson_append_int(&bb, "integer", 5);
    bson_append_double(&bb, "number", 5.05);
    bson_append_bool(&bb, "boolean", 0);

    bson_append_start_array(&bb, "array");
    bson_append_string(&bb, "0", "test");
    bson_append_string(&bb, "1", "benchmark");
    bson_append_finish_object(&bb);

    bson_from_buffer(out, &bb);
}

static const char *words[14] = 
    {"10gen","web","open","source","application","paas",
    "platform-as-a-service","technology","helps",
    "developers","focus","building","mongodb","mongo"};

static void make_large(bson * out, int i){
    int num;
    char numstr[4];
    bson_buffer bb;
    bson_buffer_init(&bb);

    bson_append_new_oid(&bb, "_id");
    bson_append_int(&bb, "x", i);
    bson_append_string(&bb, "base_url", "http://www.example.com/test-me");
    bson_append_int(&bb, "total_word_count", 6743);
    bson_append_int(&bb, "access_time", 999); /*TODO use date*/

    bson_append_start_object(&bb, "meta_tags");
    bson_append_string(&bb, "description", "i am a long description string");
    bson_append_string(&bb, "author", "Holly Man");
    bson_append_string(&bb, "dynamically_created_meta_tag", "who know\n what");
    bson_append_finish_object(&bb);

    bson_append_start_object(&bb, "page_structure");
    bson_append_int(&bb, "counted_tags", 3450);
    bson_append_int(&bb, "no_of_js_attached", 10);
    bson_append_int(&bb, "no_of_images", 6);
    bson_append_finish_object(&bb);


    bson_append_start_array(&bb, "harvested_words");
    for (num=0; num < 14*20; num++){
        sprintf(numstr, "%d", num);
        bson_append_string(&bb, numstr, words[i%14]);
    }
    bson_append_finish_object(&bb);

    bson_from_buffer(out, &bb);
}

static void serialize_small_test(){
    int i;
    bson b;
    for (i=0; i<PER_TRIAL; i++){
        make_small(&b, i);
        bson_destroy(&b);
    }
}
static void serialize_medium_test(){
    int i;
    bson b;
    for (i=0; i<PER_TRIAL; i++){
        make_medium(&b, i);
        bson_destroy(&b);
    }
}
static void serialize_large_test(){
    int i;
    bson b;
    for (i=0; i<PER_TRIAL; i++){
        make_large(&b, i);
        bson_destroy(&b);
    }
}
static void single_insert_small_test(){
    int i;
    bson b;
    for (i=0; i<PER_TRIAL; i++){
        make_small(&b, i);
        mongo_insert(&conn, DB ".single.small", &b);
        bson_destroy(&b);
    }
}

static void single_insert_medium_test(){
    int i;
    bson b;
    for (i=0; i<PER_TRIAL; i++){
        make_medium(&b, i);
        mongo_insert(&conn, DB ".single.medium", &b);
        bson_destroy(&b);
    }
}

static void single_insert_large_test(){
    int i;
    bson b;
    for (i=0; i<PER_TRIAL; i++){
        make_large(&b, i);
        mongo_insert(&conn, DB ".single.large", &b);
        bson_destroy(&b);
    }
}

static void batch_insert_small_test(){
    int i, j;
    bson b[BATCH_SIZE];
    bson *bp[BATCH_SIZE];
    for (j=0; j < BATCH_SIZE; j++)
        bp[j] = &b[j];

    for (i=0; i < (PER_TRIAL / BATCH_SIZE); i++){
        for (j=0; j < BATCH_SIZE; j++)
            make_small(&b[j], i);

        mongo_insert_batch(&conn, DB ".batch.small", bp, BATCH_SIZE);

        for (j=0; j < BATCH_SIZE; j++)
            bson_destroy(&b[j]);
    }
}

static void batch_insert_medium_test(){
    int i, j;
    bson b[BATCH_SIZE];
    bson *bp[BATCH_SIZE];
    for (j=0; j < BATCH_SIZE; j++)
        bp[j] = &b[j];

    for (i=0; i < (PER_TRIAL / BATCH_SIZE); i++){
        for (j=0; j < BATCH_SIZE; j++)
            make_medium(&b[j], i);

        mongo_insert_batch(&conn, DB ".batch.medium", bp, BATCH_SIZE);

        for (j=0; j < BATCH_SIZE; j++)
            bson_destroy(&b[j]);
    }
}

static void batch_insert_large_test(){
    int i, j;
    bson b[BATCH_SIZE];
    bson *bp[BATCH_SIZE];
    for (j=0; j < BATCH_SIZE; j++)
        bp[j] = &b[j];

    for (i=0; i < (PER_TRIAL / BATCH_SIZE); i++){
        for (j=0; j < BATCH_SIZE; j++)
            make_large(&b[j], i);

        mongo_insert_batch(&conn, DB ".batch.large", bp, BATCH_SIZE);

        for (j=0; j < BATCH_SIZE; j++)
            bson_destroy(&b[j]);
    }
}

typedef void(*nullary)();
static void time_it(nullary func, const char* name, bson_bool_t gle){
    struct timeval start, end;
    double timer;
    double ops;

    gettimeofday(&start, NULL);
    func();
    if (gle) ASSERT(!mongo_cmd_get_last_error(&conn, DB, NULL));
    gettimeofday(&end, NULL);

    timer = end.tv_sec - start.tv_sec;
    timer *= 1000000;
    timer += end.tv_usec - start.tv_usec;

    ops = PER_TRIAL / timer;
    ops *= 1000000;

    printf("%-45s\t%15f\n", name, ops);
}

#define TIME(func, gle) (time_it(func, #func, gle))

static void clean(){
    bson b;
    if (!mongo_cmd_drop_db(&conn, DB)){
        printf("failed to drop db\n");
        exit(1);
    }

    /* create the db */
    mongo_insert(&conn, DB ".creation", bson_empty(&b));
    ASSERT(!mongo_cmd_get_last_error(&conn, DB, NULL));
}

int main(){
    mongo_connection_options opts;
    
    strncpy(opts.host, TEST_SERVER, 255);
    opts.host[254] = '\0';
    opts.port = 27017;

    if (mongo_connect(&conn, &opts )){
        printf("failed to connect\n");
        exit(1);
    }

    clean();

    printf("-----\n");
    TIME(serialize_small_test, 0);
    TIME(serialize_medium_test, 0);
    TIME(serialize_large_test, 0);

    printf("-----\n");
    TIME(single_insert_small_test, 1);
    TIME(single_insert_medium_test, 1);
    TIME(single_insert_large_test, 1);

    printf("-----\n");
    TIME(batch_insert_small_test, 1);
    TIME(batch_insert_medium_test, 1);
    TIME(batch_insert_large_test, 1);

    return 0;
}
