#ifndef HAUVIE_TOKEN_STREAM_H
#define HAUVIE_TOKEN_STREAM_H

#include <stdbool.h>
#include <stddef.h>

#define HAUVIE_MAX_SOURCE_LENGTH 512
#define HAUVIE_MAX_SOURCE_TOKENS 32
#define HAUVIE_MAX_TOKEN_LENGTH 64

typedef enum {
    HAUVIE_TOKEN_WORD = 0,
    HAUVIE_TOKEN_EQUALS
} HauvieTokenKind;

typedef enum {
    HAUVIE_STREAM_TOKEN = 0,
    HAUVIE_STREAM_END,
    HAUVIE_STREAM_ERROR
} HauvieStreamResult;

typedef struct {
    HauvieTokenKind kind;
    const char *text; /* Slice into the source; not NUL-terminated. */
    size_t length;
    size_t offset;
} HauvieToken;

typedef struct {
    const char *source;
    size_t source_length;
    size_t position;
    size_t tokens_emitted;
    bool failed;
} HauvieTokenStream;

/* Initialize a bounded, non-owning stream over a NUL-terminated source. */
bool hauvie_token_stream_init(HauvieTokenStream *stream, const char *source);

/* Return one token, end-of-stream, or a terminal lexical/limit error. */
HauvieStreamResult hauvie_token_stream_next(
    HauvieTokenStream *stream,
    HauvieToken *token
);

#endif
