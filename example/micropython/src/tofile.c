#include <rtthread.h>
#ifdef RT_USING_RYM
#include <ymodem.h>
#ifdef RT_USING_DFS
    #include <dfs_posix.h>
#endif
#include <stdlib.h>

#include <board.h>

#ifdef RT_USING_POSIX

struct custom_ctx
{
    struct rym_ctx parent;
    int fd;
    int flen;
    char fpath[256];
};

static enum rym_code _rym_bg(
    struct rym_ctx *ctx,
    rt_uint8_t *buf,
    rt_size_t len)
{
    struct custom_ctx *cctx = (struct custom_ctx *)ctx;
    cctx->fpath[0] = '/';
    /* the buf should be the file name */
    strcpy(&(cctx->fpath[1]), (const char *)buf);
    cctx->fd = open(cctx->fpath, O_CREAT | O_WRONLY | O_TRUNC, 0);
    if (cctx->fd < 0)
    {
        rt_err_t err = rt_get_errno();
        rt_kprintf("error creating file: %d\n", err);
        rt_kprintf("abort transmission\n");
        return RYM_CODE_CAN;
    }

    cctx->flen = atoi((const char *)buf + strlen((const char *)buf) + 1);
    if (cctx->flen == 0)
        cctx->flen = -1;
    return RYM_CODE_ACK;
}

static enum rym_code _rym_tof(
    struct rym_ctx *ctx,
    rt_uint8_t *buf,
    rt_size_t len)
{
    struct custom_ctx *cctx = (struct custom_ctx *)ctx;
    RT_ASSERT(cctx->fd >= 0);


    if (cctx->flen == -1)
    {
        write(cctx->fd, buf, len);
    }
    else
    {
        int wlen = len > cctx->flen ? cctx->flen : len;
        write(cctx->fd, buf, wlen);
        cctx->flen -= wlen;
    }

    return RYM_CODE_ACK;
}

static enum rym_code _rym_end(
    struct rym_ctx *ctx,
    rt_uint8_t *buf,
    rt_size_t len)
{
    struct custom_ctx *cctx = (struct custom_ctx *)ctx;

    RT_ASSERT(cctx->fd >= 0);
    close(cctx->fd);
    cctx->fd = -1;

    return RYM_CODE_ACK;
}

rt_err_t rym_write_to_file(rt_device_t idev)
{
    rt_err_t res;
    struct custom_ctx *ctx = rt_malloc(sizeof(*ctx));

    RT_ASSERT(idev);

    rt_kprintf("entering RYM mode\n");

    res = rym_recv_on_device(&ctx->parent, idev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX,
                             _rym_bg, _rym_tof, _rym_end, 1000);

    /* there is no Ymodem traffic on the line so print out info. */
    rt_kprintf("leaving RYM mode with code %d\n", res);
    rt_kprintf("file %s has been created.\n", ctx->fpath);

    rt_free(ctx);

    return res;
}

#ifdef FINSH_USING_MSH
#include <finsh.h>
rt_err_t ry(int argc, char **argv)
{
    rt_err_t res;
    rt_device_t dev;

    if (argc < 1)
        return -RT_ERROR;
    if (argc == 1)
        rt_device_find(RT_CONSOLE_DEVICE_NAME);
    else
        dev = rt_device_find(argv[1]);
    if (!dev)
    {
        rt_kprintf("could not find device:%s\n", argv[1]);
        return -RT_ERROR;
    }

    res = rym_write_to_file(dev);

    return res;
}
FINSH_FUNCTION_EXPORT(ry, receive files by ymodem protocol);
MSH_CMD_EXPORT(ry, receive files by ymodem protocol);

#endif  //FINSH_USING_MSH
#endif  //RT_USING_POSIX
#endif  //RT_USING_RYM
