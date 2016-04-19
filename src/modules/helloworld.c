#include "../redismodule.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* HELLO.SIMPLE is among the simplest commands you can implement.
 * It just returns the currently selected DB id, a functionality which is
 * missing in Redis. The command uses two important API calls: one to
 * fetch the currently selected DB, the other in order to send the client
 * an integer reply as response. */
int HelloSimple_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_ReplyWithLongLong(ctx,RedisModule_GetSelectedDb(ctx));
    return REDISMODULE_OK;
}

/* HELLO.PUSH.NATIVE re-implements RPUSH, and shows the low level modules API
 * where you can "open" keys, make low level operations, create new keys by
 * pushing elements into non-existing keys, and so forth.
 *
 * You'll find this command to be roughly as fast as the actual RPUSH
 * command. */
int HelloPushNative_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) return RedisModule_WrongArity(ctx);

    RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],
        REDISMODULE_READ|REDISMODULE_WRITE);

    RedisModule_ListPush(key,REDISMODULE_LIST_TAIL,argv[2]);
    size_t newlen = RedisModule_ValueLength(key);
    RedisModule_CloseKey(key);
    RedisModule_ReplyWithLongLong(ctx,newlen);
    return REDISMODULE_OK;
}

/* HELLO.PUSH.CALL implements RPUSH using an higher level approach, calling
 * a Redis command instead of working with the key in a low level way. This
 * approach is useful when you need to call Redis commands that are not
 * available as low level APIs, or when you don't need the maximum speed
 * possible but instead prefer implementation simplicity. */
int HelloPushCall_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) return RedisModule_WrongArity(ctx);

    RedisModuleCallReply *reply;

    reply = RedisModule_Call(ctx,"RPUSH","ss",argv[1],argv[2]);
    long long len = RedisModule_CallReplyInteger(reply);
    RedisModule_FreeCallReply(reply);
    RedisModule_ReplyWithLongLong(ctx,len);
    return REDISMODULE_OK;
}

/* HELLO.PUSH.CALL2
 * This is exaxctly as HELLO.PUSH.CALL, but shows how we can reply to the
 * client using directly a reply object that Call() returned. */
int HelloPushCall2_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 3) return RedisModule_WrongArity(ctx);

    RedisModuleCallReply *reply;

    reply = RedisModule_Call(ctx,"RPUSH","ss",argv[1],argv[2]);
    RedisModule_ReplyWithCallReply(ctx,reply);
    RedisModule_FreeCallReply(reply);
    return REDISMODULE_OK;
}

/* HELLO.LIST.SUM.LEN returns the total length of all the items inside
 * a Redis list, by using the high level Call() API.
 * This command is an example of the array reply access. */
int HelloListSumLen_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 2) return RedisModule_WrongArity(ctx);

    RedisModuleCallReply *reply;

    reply = RedisModule_Call(ctx,"LRANGE","sll",argv[1],(long long)0,(long long)-1);
    size_t strlen = 0;
    size_t items = RedisModule_CallReplyLength(reply);
    size_t j;
    for (j = 0; j < items; j++) {
        RedisModuleCallReply *ele = RedisModule_CallReplyArrayElement(reply,j);
        strlen += RedisModule_CallReplyLength(ele);
    }
    RedisModule_FreeCallReply(reply);
    RedisModule_ReplyWithLongLong(ctx,strlen);
    return REDISMODULE_OK;
}

/* HELLO.LIST.SPLICE srclist dstlist count
 * Moves 'count' elements from the tail of 'srclist' to the head of
 * 'dstlist'. If less than count elements are available, it moves as much
 * elements as possible. */
int HelloListSplice_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 4) return RedisModule_WrongArity(ctx);

    RedisModuleKey *srckey = RedisModule_OpenKey(ctx,argv[1],
        REDISMODULE_READ|REDISMODULE_WRITE);
    RedisModuleKey *dstkey = RedisModule_OpenKey(ctx,argv[2],
        REDISMODULE_READ|REDISMODULE_WRITE);

    /* Src and dst key must be empty or lists. */
    if ((RedisModule_KeyType(srckey) != REDISMODULE_KEYTYPE_LIST &&
         RedisModule_KeyType(srckey) != REDISMODULE_KEYTYPE_EMPTY) ||
        (RedisModule_KeyType(dstkey) != REDISMODULE_KEYTYPE_LIST &&
         RedisModule_KeyType(dstkey) != REDISMODULE_KEYTYPE_EMPTY))
    {
        RedisModule_CloseKey(srckey);
        RedisModule_CloseKey(dstkey);
        return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);
    }

    long long count;
    if (RedisModule_StringToLongLong(argv[3],&count) != REDISMODULE_OK) {
        RedisModule_CloseKey(srckey);
        RedisModule_CloseKey(dstkey);
        return RedisModule_ReplyWithError(ctx,"ERR invalid count");
    }

    while(count-- > 0) {
        RedisModuleString *ele;

        ele = RedisModule_ListPop(srckey,REDISMODULE_LIST_TAIL);
        if (ele == NULL) break;
        RedisModule_ListPush(dstkey,REDISMODULE_LIST_HEAD,ele);
        RedisModule_FreeString(ctx,ele);
    }

    size_t len = RedisModule_ValueLength(srckey);
    RedisModule_CloseKey(srckey);
    RedisModule_CloseKey(dstkey);
    RedisModule_ReplyWithLongLong(ctx,len);
    return REDISMODULE_OK;
}

/* Like the HELLO.LIST.SPLICE above, but uses automatic memory management
 * in order to avoid freeing stuff. */
int HelloListSpliceAuto_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 4) return RedisModule_WrongArity(ctx);

    RedisModule_AutoMemory(ctx);

    RedisModuleKey *srckey = RedisModule_OpenKey(ctx,argv[1],
        REDISMODULE_READ|REDISMODULE_WRITE);
    RedisModuleKey *dstkey = RedisModule_OpenKey(ctx,argv[2],
        REDISMODULE_READ|REDISMODULE_WRITE);

    /* Src and dst key must be empty or lists. */
    if ((RedisModule_KeyType(srckey) != REDISMODULE_KEYTYPE_LIST &&
         RedisModule_KeyType(srckey) != REDISMODULE_KEYTYPE_EMPTY) ||
        (RedisModule_KeyType(dstkey) != REDISMODULE_KEYTYPE_LIST &&
         RedisModule_KeyType(dstkey) != REDISMODULE_KEYTYPE_EMPTY))
    {
        return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);
    }

    long long count;
    if (RedisModule_StringToLongLong(argv[3],&count) != REDISMODULE_OK)
        return RedisModule_ReplyWithError(ctx,"ERR invalid count");

    while(count-- > 0) {
        RedisModuleString *ele;

        ele = RedisModule_ListPop(srckey,REDISMODULE_LIST_TAIL);
        if (ele == NULL) break;
        RedisModule_ListPush(dstkey,REDISMODULE_LIST_HEAD,ele);
    }

    size_t len = RedisModule_ValueLength(srckey);
    RedisModule_ReplyWithLongLong(ctx,len);
    return REDISMODULE_OK;
}

/* HELLO.RAND.ARRAY <count>
 * Shows how to generate arrays as commands replies.
 * It just outputs <count> random numbers. */
int HelloRandArray_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) return RedisModule_WrongArity(ctx);
    long long count;
    if (RedisModule_StringToLongLong(argv[1],&count) != REDISMODULE_OK ||
        count < 0)
        return RedisModule_ReplyWithError(ctx,"ERR invalid count");

    /* To reply with an array, we call RedisModule_ReplyWithArray() followed
     * by other "count" calls to other reply functions in order to generate
     * the elements of the array. */
    RedisModule_ReplyWithArray(ctx,count);
    while(count--) RedisModule_ReplyWithLongLong(ctx,rand());
    return REDISMODULE_OK;
}

/* This is a simple command to test replication. Because of the "!" modified
 * in the RedisModule_Call() call, the two INCRs get replicated.
 * Also note how the ECHO is replicated in an unexpected position (check
 * comments the function implementation). */
int HelloRepl1_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    RedisModuleCallReply *reply;
    RedisModule_AutoMemory(ctx);

    /* This will be replicated *after* the two INCR statements, since
     * the Call() replication has precedence, so the actual replication
     * stream will be:
     *
     * MULTI
     * INCR foo
     * INCR bar
     * ECHO c foo
     * EXEC
     */
    RedisModule_Replicate(ctx,"ECHO","c","foo");

    /* Using the "!" modifier we replicate the command if it
     * modified the dataset in some way. */
    reply = RedisModule_Call(ctx,"INCR","c!","foo");
    reply = RedisModule_Call(ctx,"INCR","c!","bar");

    RedisModule_ReplyWithLongLong(ctx,0);

    return REDISMODULE_OK;
}

/* Another command to show replication. In this case, we call
 * RedisModule_ReplicateVerbatim() to mean we want just the command to be
 * propagated to slaves / AOF exactly as it was called by the user.
 *
 * This command also shows how to work with string objects.
 * It takes a list, and increments all the elements (that must have
 * a numerical value) by 1, returning the sum of all the elements
 * as reply.
 *
 * Usage: HELLO.REPL2 <list-key> */
int HelloRepl2_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) return RedisModule_WrongArity(ctx);

    RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
    RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],
        REDISMODULE_READ|REDISMODULE_WRITE);

    if (RedisModule_KeyType(key) != REDISMODULE_KEYTYPE_LIST)
        return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);

    size_t listlen = RedisModule_ValueLength(key);
    long long sum = 0;

    /* Rotate and increment. */
    while(listlen--) {
        RedisModuleString *ele = RedisModule_ListPop(key,REDISMODULE_LIST_TAIL);
        long long val;
        if (RedisModule_StringToLongLong(ele,&val) != REDISMODULE_OK) val = 0;
        val++;
        sum += val;
        RedisModuleString *newele = RedisModule_CreateStringFromLongLong(ctx,val);
        RedisModule_ListPush(key,REDISMODULE_LIST_HEAD,newele);
    }
    RedisModule_ReplyWithLongLong(ctx,sum);
    RedisModule_ReplicateVerbatim(ctx);
    return REDISMODULE_OK;
}

/* This is an example of strings DMA access. Given a key containing a string
 * it toggles the case of each character from lower to upper case or the
 * other way around.
 *
 * No automatic memory management is used in this example (for the sake
 * of variety).
 *
 * HELLO.TOGGLE.CASE key */
int HelloToggleCase_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) return RedisModule_WrongArity(ctx);

    RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],
        REDISMODULE_READ|REDISMODULE_WRITE);

    int keytype = RedisModule_KeyType(key);
    if (keytype != REDISMODULE_KEYTYPE_STRING &&
        keytype != REDISMODULE_KEYTYPE_EMPTY)
    {
        RedisModule_CloseKey(key);
        return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);
    }

    if (keytype == REDISMODULE_KEYTYPE_STRING) {
        size_t len, j;
        char *s = RedisModule_StringDMA(key,&len,REDISMODULE_WRITE);
        for (j = 0; j < len; j++) {
            if (isupper(s[j])) {
                s[j] = tolower(s[j]);
            } else {
                s[j] = toupper(s[j]);
            }
        }
    }

    RedisModule_CloseKey(key);
    RedisModule_ReplyWithSimpleString(ctx,"OK");
    RedisModule_ReplicateVerbatim(ctx);
    return REDISMODULE_OK;
}

/* HELLO.MORE.EXPIRE key milliseconds.
 *
 * If they key has already an associated TTL, extends it by "milliseconds"
 * milliseconds. Otherwise no operation is performed. */
int HelloMoreExpire_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
    if (argc != 3) return RedisModule_WrongArity(ctx);

    mstime_t addms, expire;

    if (RedisModule_StringToLongLong(argv[2],&addms) != REDISMODULE_OK)
        return RedisModule_ReplyWithError(ctx,"ERR invalid expire time");

    RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],
        REDISMODULE_READ|REDISMODULE_WRITE);
    expire = RedisModule_GetExpire(key);
    if (expire != REDISMODULE_NO_EXPIRE) {
        expire += addms;
        RedisModule_SetExpire(key,expire);
    }
    return RedisModule_ReplyWithSimpleString(ctx,"OK");
}

/* HELLO.ZSUMRANGE key startscore endscore
 * Return the sum of all the scores elements between startscore and endscore. */
int HelloZsumRange_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModuleZsetRange zrange = REDISMODULE_ZSET_RANGE_INIT;
    zrange.type = REDISMODULE_ZSET_RANGE_SCORE;
    if (RedisModule_StringToDouble(argv[2],&zrange.score_start) != REDISMODULE_OK ||
        RedisModule_StringToDouble(argv[3],&zrange.score_end) != REDISMODULE_OK)
    {
        return RedisModule_ReplyWithError(ctx,"ERR invalid range");
    }
    zrange.flags = 0;

    RedisModuleKey *key = RedisModule_OpenKey(ctx,argv[1],
        REDISMODULE_READ|REDISMODULE_WRITE);
    if (RedisModule_KeyType(key) != REDISMODULE_KEYTYPE_ZSET) {
        return RedisModule_ReplyWithError(ctx,REDISMODULE_ERRORMSG_WRONGTYPE);
    }

    RedisModule_ZsetFirstInRange(key,&zrange);
    double scoresum = 0;
    while(!RedisModule_ZsetRangeEndReached(key)) {
        double score;
        RedisModuleString *ele = RedisModule_ZsetRangeCurrentElement(key,&score);
        RedisModule_FreeString(ctx,ele);
        scoresum += score;
        RedisModule_ZsetRangeNext(key);
    }
    RedisModule_ZsetRangeStop(key);
    RedisModule_CloseKey(key);
    RedisModule_ReplyWithDouble(ctx,scoresum);
    return REDISMODULE_OK;
}

/* This function must be present on each Redis module. It is used in order to
 * register the commands into the Redis server. */
int RedisModule_OnLoad(RedisModuleCtx *ctx) {
    if (RedisModule_Init(ctx,"helloworld",1,REDISMODULE_APIVER_1)
        == REDISMODULE_ERR) return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.simple",
        HelloSimple_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.push.native",
        HelloPushNative_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.push.call",
        HelloPushCall_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.push.call2",
        HelloPushCall2_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.list.sum.len",
        HelloListSumLen_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.list.splice",
        HelloListSplice_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.list.splice.auto",
        HelloListSpliceAuto_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.rand.array",
        HelloRandArray_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.repl1",
        HelloRepl1_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.repl2",
        HelloRepl2_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.toggle.case",
        HelloToggleCase_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.more.expire",
        HelloMoreExpire_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"hello.zsumrange",
        HelloZsumRange_RedisCommand) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}
