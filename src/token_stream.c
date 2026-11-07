#include "hauvie_token_stream.h"

#include <string.h>

static bool is_space(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

static bool is_word_character(char ch) {
    return (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9') ||
           ch == '_' || ch == '-' || ch == '.';
}

bool hauvie_token_stream_init(HauvieTokenStream *stream, const char *source) {
    if (stream == NULL) {
        return false;
    }

    *stream = (HauvieTokenStream){0};
    if (source == NULL) {
        stream->failed = true;
        return false;
    }

    size_t length = strlen(source);
    if (length == 0 || length > HAUVIE_MAX_SOURCE_LENGTH) {
        stream->failed = true;
        return false;
    }

    stream->source = source;
    stream->source_length = length;
    return true;
}

HauvieStreamResult hauvie_token_stream_next(
    HauvieTokenStream *stream,
    HauvieToken *token
) {
    if (stream == NULL || token == NULL || stream->failed ||
        stream->source == NULL) {
        return HAUVIE_STREAM_ERROR;
    }

    while (stream->position < stream->source_length &&
           is_space(stream->source[stream->position])) {
        ++stream->position;
    }
    if (stream->position == stream->source_length) {
        return HAUVIE_STREAM_END;
    }
    if (stream->tokens_emitted >= HAUVIE_MAX_SOURCE_TOKENS) {
        stream->failed = true;
        return HAUVIE_STREAM_ERROR;
    }

    size_t start = stream->position;
    if (stream->source[stream->position] == '=') {
        token->kind = HAUVIE_TOKEN_EQUALS;
        token->text = stream->source + start;
        token->length = 1;
        token->offset = start;
        ++stream->position;
    } else {
        while (stream->position < stream->source_length &&
               is_word_character(stream->source[stream->position])) {
            ++stream->position;
        }
        size_t length = stream->position - start;
        if (length == 0 || length > HAUVIE_MAX_TOKEN_LENGTH) {
            stream->failed = true;
            return HAUVIE_STREAM_ERROR;
        }
        token->kind = HAUVIE_TOKEN_WORD;
        token->text = stream->source + start;
        token->length = length;
        token->offset = start;
    }

    ++stream->tokens_emitted;
    return HAUVIE_STREAM_TOKEN;
}
