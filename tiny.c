#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "include/libstream.h"

int get_flags(const char *mode)
{
    if (!strcmp(mode, "r"))
        return O_RDONLY;
    if (!strcmp(mode, "w"))
        return O_WRONLY | O_CREAT | O_TRUNC;
    if (!strcmp(mode, "r+"))
        return O_RDWR;
    if (!strcmp(mode, "w+"))
        return O_RDWR | O_CREAT | O_TRUNC;
    return -1;
}

struct stream *lbs_fdopen(int fd, const char *mode)
{
    if (fd < 0)
        return NULL;
    struct stream *str = malloc(sizeof(struct stream));
    if (str == NULL)
        return NULL;
    int flag = get_flags(mode);
    if (flag == -1)
        return NULL;
    str->fd = fd;
    str->flags = flag;
    str->error = 0;
    str->buffered_size = 0;
    str->already_read = 0;
    if (flag)
        str->buffering_mode = STREAM_LINE_BUFFERED;
    else
        str->buffering_mode = STREAM_BUFFERED;
    return str;
}

struct stream *lbs_fopen(const char *path, const char *mode)
{
    int flag = get_flags(mode);
    int fd = open(path, flag);
    if (fd == -1)
        return NULL;
    return lbs_fdopen(fd, mode);
}

int lbs_fflush(struct stream *stream)
{
    if (stream == NULL)
        return 1;
    if (stream->io_operation == STREAM_READING)
    {
        size_t curr = stream_remaining_buffered(stream);
        if (curr)
        {
            lseek(stream->fd, -curr, SEEK_CUR);
        }
        stream->already_read = 0;
        stream->buffered_size = 0;
    }
    else if (stream->io_operation == STREAM_WRITING)
    {
        if (stream->buffered_size)
        {
            size_t size = stream->buffered_size;
            ssize_t writ = write(stream->fd, stream->buffer, size);
            if (writ == -1)
            {
                stream->error = 1;
                return LBS_EOF;
            }
            stream->buffered_size = 0;
        }
    }
    return 0;
}

int lbs_fclose(struct stream *file)
{
    if (lbs_fflush(file) == 1)
        return -1;
    int clos = close(file->fd);
    if (clos == -1)
        return -1;
    free(file);
    return 0;
}

int lbs_fputc(int c, struct stream *stream)
{
    if (!stream_writable(stream))
    {
        stream->error = 1;
        return -1;
    }
    if (stream->io_operation != STREAM_WRITING)
    {
        lbs_fflush(stream);
    }

    stream->io_operation = STREAM_WRITING;

    stream->buffer[stream->buffered_size] = c;
    stream->buffered_size = stream->buffered_size + 1;

    if ((stream->buffering_mode == STREAM_UNBUFFERED
         || stream_unused_buffer_space(stream) == 0
         || (stream->buffering_mode == STREAM_LINE_BUFFERED && c == '\n')))
        lbs_fflush(stream);
    if (stream->error == 1)
        return -1;
    else
        return c;
}
/*int lbs_fputc(int c, struct stream *stream)
{
    if (stream == NULL)
        return -1;
    if (!stream_writable(stream))
    {
        stream->error = 1;
        return -1;
    }
    if (stream->io_operation == STREAM_READING)
    {
        lbs_fflush(stream);
    }
    stream->io_operation = STREAM_WRITING;
    stream->buffer[stream->buffered_size] = c;
    stream->buffered_size = stream->buffered_size + 1;
    if (stream->buffered_size == LBS_BUFFER_SIZE
        || stream->buffering_mode == STREAM_BUFFERED
        || ((stream->buffering_mode == STREAM_LINE_BUFFERED) && (c == '\n')))
    {
        lbs_fflush(stream);
    }
    if (stream->error == 1)
        return -1;
    return c;
}*/

int lbs_fgetc(struct stream *stream)
{
    if (!stream_readable(stream))
        return -1;
    if (stream->io_operation != STREAM_READING)
    {
        if (lbs_fflush(stream) == -1)
            return -1;
        stream->io_operation = STREAM_READING;
    }
    if (stream_remaining_buffered(stream) == 0)
    {
        int nb = read(stream->fd, stream->buffer, LBS_BUFFER_SIZE);
        if (nb == -1)
        {
            stream->error = 1;
            return -1;
        }
        if (nb == 0)
            return -1;
        stream->buffered_size = nb;
        stream->already_read = 0;
    }
    if (stream->buffered_size != 0)
        return (unsigned char)(stream->buffer[(stream->already_read++)]);
    return 0;
}
