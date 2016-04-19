#ifndef REDISMODULE_H
#define REDISMODULE_H

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>

/* ---------------- Defines common between core and modules --------------- */

/* Error status return values. */
#define REDISMODULE_OK 0
#define REDISMODULE_ERR 1

/* API versions. */
#define REDISMODULE_APIVER_1 1

/* API flags and constants */
#define REDISMODULE_READ (1<<0)
#define REDISMODULE_WRITE (1<<1)

#define REDISMODULE_LIST_HEAD 0
#define REDISMODULE_LIST_TAIL 1

/* Key types. */
#define REDISMODULE_KEYTYPE_EMPTY 0
#define REDISMODULE_KEYTYPE_STRING 1
#define REDISMODULE_KEYTYPE_LIST 2
#define REDISMODULE_KEYTYPE_HASH 3
#define REDISMODULE_KEYTYPE_SET 4
#define REDISMODULE_KEYTYPE_ZSET 5

/* Reply types. */
#define REDISMODULE_REPLY_UNKNOWN -1
#define REDISMODULE_REPLY_STRING 0
#define REDISMODULE_REPLY_ERROR 1
#define REDISMODULE_REPLY_INTEGER 2
#define REDISMODULE_REPLY_ARRAY 3
#define REDISMODULE_REPLY_NULL 4

/* Expire */
#define REDISMODULE_NO_EXPIRE -1

/* Sorted set API flags. */
#define REDISMODULE_ZADD_XX      (1<<0)
#define REDISMODULE_ZADD_NX      (1<<1)
#define REDISMODULE_ZADD_ADDED   (1<<2)
#define REDISMODULE_ZADD_UPDATED (1<<3)
#define REDISMODULE_ZADD_NOP     (1<<4)

/* Error messages. */
#define REDISMODULE_ERRORMSG_WRONGTYPE "WRONGTYPE Operation against a key holding the wrong kind of value"

/* Sorted set range structure. */
typedef struct RedisModuleZsetRange {
    uint32_t type;
    uint32_t flags;
    double score_start;
    double score_end;
    char *lex_start;
    char *lex_end;
    uint32_t lex_start_len;
    uint32_t lex_end_len;
    uint32_t pos_start;
    uint32_t pos_end;
} RedisModuleZsetRange;

#define REDISMODULE_POSITIVE_INFINITE (1.0/0.0)
#define REDISMODULE_NEGATIVE_INFINITE (-1.0/0.0)

#define REDISMODULE_ZSET_RANGE_INIT {0,0,REDISMODULE_NEGATIVE_INFINITE,REDISMODULE_POSITIVE_INFINITE,"-","+",1,1,0,-1}
#define REDISMODULE_ZSET_RANGE_LEX 1
#define REDISMODULE_ZSET_RANGE_SCORE 2
#define REDISMODULE_ZSET_RANGE_POS 3

#define REDISMODULE_ZSET_RANGE_START_EX (1<<0)
#define REDISMODULE_ZSET_RANGE_END_EX (1<<1)

/* ------------------------- End of common defines ------------------------ */

#ifndef REDISMODULE_CORE

typedef long long mstime_t;

/* Incomplete structures for compiler checks but opaque access. */
typedef struct RedisModuleCtx RedisModuleCtx;
typedef struct RedisModuleKey RedisModuleKey;
typedef struct RedisModuleString RedisModuleString;
typedef struct RedisModuleCallReply RedisModuleCallReply;

typedef int (*RedisModuleCmdFunc) (RedisModuleCtx *ctx, RedisModuleString **argv, int argc);

#define REDISMODULE_GET_API(name) \
    RedisModule_GetApi("RedisModule_" #name, ((void **)&RedisModule_ ## name))

#define REDISMODULE_API_FUNC(x) (*x)

int REDISMODULE_API_FUNC(RedisModule_GetApi)(const char *, void *);
int REDISMODULE_API_FUNC(RedisModule_CreateCommand)(RedisModuleCtx *ctx, const char *name, RedisModuleCmdFunc cmdfunc);
int REDISMODULE_API_FUNC(RedisModule_SetModuleAttribs)(RedisModuleCtx *ctx, const char *name, int ver, int apiver);
int REDISMODULE_API_FUNC(RedisModule_WrongArity)(RedisModuleCtx *ctx);
int REDISMODULE_API_FUNC(RedisModule_ReplyWithLongLong)(RedisModuleCtx *ctx, long long ll);
int REDISMODULE_API_FUNC(RedisModule_GetSelectedDb)(RedisModuleCtx *ctx);
int REDISMODULE_API_FUNC(RedisModule_SelectDb)(RedisModuleCtx *ctx, int newid);
void *REDISMODULE_API_FUNC(RedisModule_OpenKey)(RedisModuleCtx *ctx, RedisModuleString *keyname, int mode);
void REDISMODULE_API_FUNC(RedisModule_CloseKey)(RedisModuleKey *kp);
int REDISMODULE_API_FUNC(RedisModule_KeyType)(RedisModuleKey *kp);
size_t REDISMODULE_API_FUNC(RedisModule_ValueLength)(RedisModuleKey *kp);
int REDISMODULE_API_FUNC(RedisModule_ListPush)(RedisModuleKey *kp, int where, RedisModuleString *ele);
RedisModuleString *REDISMODULE_API_FUNC(RedisModule_ListPop)(RedisModuleKey *key, int where);
RedisModuleCallReply *REDISMODULE_API_FUNC(RedisModule_Call)(RedisModuleCtx *ctx, const char *cmdname, const char *fmt, ...);
const char *REDISMODULE_API_FUNC(RedisModule_CallReplyProto)(RedisModuleCallReply *reply, size_t *len);
void REDISMODULE_API_FUNC(RedisModule_FreeCallReply)(RedisModuleCallReply *reply);
int REDISMODULE_API_FUNC(RedisModule_CallReplyType)(RedisModuleCallReply *reply);
long long REDISMODULE_API_FUNC(RedisModule_CallReplyInteger)(RedisModuleCallReply *reply);
size_t REDISMODULE_API_FUNC(RedisModule_CallReplyLength)(RedisModuleCallReply *reply);
RedisModuleCallReply *REDISMODULE_API_FUNC(RedisModule_CallReplyArrayElement)(RedisModuleCallReply *reply, size_t idx);
RedisModuleString *REDISMODULE_API_FUNC(RedisModule_CreateString)(RedisModuleCtx *ctx, const char *ptr, size_t len);
RedisModuleString *REDISMODULE_API_FUNC(RedisModule_CreateStringFromLongLong)(RedisModuleCtx *ctx, long long ll);
void REDISMODULE_API_FUNC(RedisModule_FreeString)(RedisModuleCtx *ctx, RedisModuleString *str);
const char *REDISMODULE_API_FUNC(RedisModule_StringPtrLen)(RedisModuleString *str, size_t *len);
int REDISMODULE_API_FUNC(RedisModule_ReplyWithError)(RedisModuleCtx *ctx, const char *err);
int REDISMODULE_API_FUNC(RedisModule_ReplyWithSimpleString)(RedisModuleCtx *ctx, const char *msg);
int REDISMODULE_API_FUNC(RedisModule_ReplyWithArray)(RedisModuleCtx *ctx, int len);
int REDISMODULE_API_FUNC(RedisModule_ReplyWithStringBuffer)(RedisModuleCtx *ctx, const char *buf, size_t len);
int REDISMODULE_API_FUNC(RedisModule_ReplyWithString)(RedisModuleCtx *ctx, RedisModuleString *str);
int REDISMODULE_API_FUNC(RedisModule_ReplyWithNull)(RedisModuleCtx *ctx);
int REDISMODULE_API_FUNC(RedisModule_ReplyWithDouble)(RedisModuleCtx *ctx, double d);
int REDISMODULE_API_FUNC(RedisModule_ReplyWithCallReply)(RedisModuleCtx *ctx, RedisModuleCallReply *reply);
int REDISMODULE_API_FUNC(RedisModule_StringToLongLong)(RedisModuleString *str, long long *ll);
int REDISMODULE_API_FUNC(RedisModule_StringToDouble)(RedisModuleString *str, double *d);
void REDISMODULE_API_FUNC(RedisModule_AutoMemory)(RedisModuleCtx *ctx);
int REDISMODULE_API_FUNC(RedisModule_Replicate)(RedisModuleCtx *ctx, const char *cmdname, const char *fmt, ...);
int REDISMODULE_API_FUNC(RedisModule_ReplicateVerbatim)(RedisModuleCtx *ctx);
const char *REDISMODULE_API_FUNC(RedisModule_CallReplyStringPtr)(RedisModuleCallReply *reply, size_t *len);
RedisModuleString *REDISMODULE_API_FUNC(RedisModule_CreateStringFromCallReply)(RedisModuleCallReply *reply);
int REDISMODULE_API_FUNC(RedisModule_DeleteKey)(RedisModuleKey *key);
int REDISMODULE_API_FUNC(RedisModule_StringSet)(RedisModuleKey *key, RedisModuleString *str);
char *REDISMODULE_API_FUNC(RedisModule_StringDMA)(RedisModuleKey *key, size_t *len, int mode);
int REDISMODULE_API_FUNC(RedisModule_StringTruncate)(RedisModuleKey *key, size_t newlen);
mstime_t REDISMODULE_API_FUNC(RedisModule_GetExpire)(RedisModuleKey *key);
int REDISMODULE_API_FUNC(RedisModule_SetExpire)(RedisModuleKey *key, mstime_t expire);
int REDISMODULE_API_FUNC(RedisModule_ZsetAdd)(RedisModuleKey *key, double score, RedisModuleString *ele, int *flagsptr);
int REDISMODULE_API_FUNC(RedisModule_ZsetIncrby)(RedisModuleKey *key, double score, RedisModuleString *ele, int *flagsptr, double *newscore);
int REDISMODULE_API_FUNC(RedisModule_ZsetScore)(RedisModuleKey *key, RedisModuleString *ele, double *score);
int REDISMODULE_API_FUNC(RedisModule_ZsetRem)(RedisModuleKey *key, RedisModuleString *ele, int *deleted);
void REDISMODULE_API_FUNC(RedisModule_ZsetRangeStop)(RedisModuleKey *key);
int REDISMODULE_API_FUNC(RedisModule_ZsetFirstInRange)(RedisModuleKey *key, RedisModuleZsetRange *zr);
RedisModuleString *REDISMODULE_API_FUNC(RedisModule_ZsetRangeCurrentElement)(RedisModuleKey *key, double *score);
int REDISMODULE_API_FUNC(RedisModule_ZsetRangeNext)(RedisModuleKey *key);
int REDISMODULE_API_FUNC(RedisModule_ZsetRangeEndReached)(RedisModuleKey *key);

/* This is included inline inside each Redis module. */
static int RedisModule_Init(RedisModuleCtx *ctx, const char *name, int ver, int apiver) {
    void *getapifuncptr = ((void**)ctx)[0];
    RedisModule_GetApi = (int (*)(const char *, void *)) (unsigned long)getapifuncptr;
    REDISMODULE_GET_API(CreateCommand);
    REDISMODULE_GET_API(SetModuleAttribs);
    REDISMODULE_GET_API(WrongArity);
    REDISMODULE_GET_API(ReplyWithLongLong);
    REDISMODULE_GET_API(ReplyWithError);
    REDISMODULE_GET_API(ReplyWithSimpleString);
    REDISMODULE_GET_API(ReplyWithArray);
    REDISMODULE_GET_API(ReplyWithStringBuffer);
    REDISMODULE_GET_API(ReplyWithString);
    REDISMODULE_GET_API(ReplyWithNull);
    REDISMODULE_GET_API(ReplyWithCallReply);
    REDISMODULE_GET_API(ReplyWithDouble);
    REDISMODULE_GET_API(GetSelectedDb);
    REDISMODULE_GET_API(SelectDb);
    REDISMODULE_GET_API(OpenKey);
    REDISMODULE_GET_API(CloseKey);
    REDISMODULE_GET_API(KeyType);
    REDISMODULE_GET_API(ValueLength);
    REDISMODULE_GET_API(ListPush);
    REDISMODULE_GET_API(ListPop);
    REDISMODULE_GET_API(StringToLongLong);
    REDISMODULE_GET_API(StringToDouble);
    REDISMODULE_GET_API(Call);
    REDISMODULE_GET_API(CallReplyProto);
    REDISMODULE_GET_API(FreeCallReply);
    REDISMODULE_GET_API(CallReplyInteger);
    REDISMODULE_GET_API(CallReplyType);
    REDISMODULE_GET_API(CallReplyLength);
    REDISMODULE_GET_API(CallReplyArrayElement);
    REDISMODULE_GET_API(CallReplyStringPtr);
    REDISMODULE_GET_API(CreateStringFromCallReply);
    REDISMODULE_GET_API(CreateString);
    REDISMODULE_GET_API(CreateStringFromLongLong);
    REDISMODULE_GET_API(FreeString);
    REDISMODULE_GET_API(StringPtrLen);
    REDISMODULE_GET_API(AutoMemory);
    REDISMODULE_GET_API(Replicate);
    REDISMODULE_GET_API(ReplicateVerbatim);
    REDISMODULE_GET_API(DeleteKey);
    REDISMODULE_GET_API(StringSet);
    REDISMODULE_GET_API(StringDMA);
    REDISMODULE_GET_API(StringTruncate);
    REDISMODULE_GET_API(GetExpire);
    REDISMODULE_GET_API(SetExpire);
    REDISMODULE_GET_API(ZsetAdd);
    REDISMODULE_GET_API(ZsetIncrby);
    REDISMODULE_GET_API(ZsetScore);
    REDISMODULE_GET_API(ZsetRem);
    REDISMODULE_GET_API(ZsetRangeStop);
    REDISMODULE_GET_API(ZsetFirstInRange);
    REDISMODULE_GET_API(ZsetRangeCurrentElement);
    REDISMODULE_GET_API(ZsetRangeNext);
    REDISMODULE_GET_API(ZsetRangeEndReached);

    RedisModule_SetModuleAttribs(ctx,name,ver,apiver);
    return REDISMODULE_OK;
}

#else

/* Things only defined for the modules core, not exported to modules
 * including this file. */
#define RedisModuleString robj

#endif /* REDISMODULE_CORE */
#endif /* REDISMOUDLE_H */
