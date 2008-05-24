 /*
  * UAE - The Un*x Amiga Emulator
  *
  * uae.resource
  *
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "traps.h"
#include "autoconf.h"
#include "execlib.h"
#include "uaeresource.h"

#if 0
  struct uaebase
  {
    struct Library lib;
    UWORD uae_version;
    UWORD uae_revision;
    UWORD uae_subrevision;
    UWORD zero;
    APTR uae_rombase;
  };
#endif

static uaecptr res_init, res_name, res_id;

static uae_u32 REGPARAM2 res_close (TrapContext *context)
{
    uaecptr base = m68k_areg (&context->regs, 6);
    put_word (base + 32, get_word (base + 32) - 1);
    return 0;
}
static uae_u32 REGPARAM2 res_open (TrapContext *context)
{
    uaecptr base = m68k_areg (&context->regs, 6);
    put_word (base + 32, get_word (base + 32) + 1);
    return 0;
}
static uae_u32 REGPARAM2 res_expunge (TrapContext *context)
{
    return 0;
}
static uae_u32 REGPARAM2 res_initcode (TrapContext *context)
{
    uaecptr base = m68k_dreg (&context->regs, 0);
    uaecptr rb = base + SIZEOF_LIBRARY;
    put_word (rb + 0, UAEMAJOR);
    put_word (rb + 2, UAEMINOR);
    put_word (rb + 4, UAESUBREV);
    put_word (rb + 6, 0);
    put_long (rb + 8, rtarea_base);
    return base;
}

uaecptr uaeres_startup (uaecptr resaddr)
{
    put_word (resaddr + 0x0, 0x4AFC);
    put_long (resaddr + 0x2, resaddr);
    put_long (resaddr + 0x6, resaddr + 0x1A); /* Continue scan here */
    put_word (resaddr + 0xA, 0x8101); /* RTF_AUTOINIT|RTF_COLDSTART; Version 1 */
    put_word (resaddr + 0xC, 0x0878); /* NT_DEVICE; pri 05 */
    put_long (resaddr + 0xE, res_name);
    put_long (resaddr + 0x12, res_id);
    put_long (resaddr + 0x16, res_init);
    resaddr += 0x1A;
    return resaddr;
}

void uaeres_install (void)
{
    uae_u32 functable, datatable;
    uae_u32 initcode, openfunc, closefunc, expungefunc;
    char tmp[100];

    sprintf (tmp, "UAE resource %d.%d.%d", UAEMAJOR, UAEMINOR, UAESUBREV);
    res_name = ds ("uae.resource");
    res_id = ds (tmp);

    /* initcode */
    initcode = here ();
    calltrap (deftrap (res_initcode)); dw (RTS);
    /* Open */
    openfunc = here ();
    calltrap (deftrap (res_open)); dw (RTS);
    /* Close */
    closefunc = here ();
    calltrap (deftrap (res_close)); dw (RTS);
    /* Expunge */
    expungefunc = here ();
    calltrap (deftrap (res_expunge)); dw (RTS);

    /* FuncTable */
    functable = here ();
    dl (openfunc); /* Open */
    dl (closefunc); /* Close */
    dl (expungefunc); /* Expunge */
    dl (EXPANSION_nullfunc); /* Null */
    dl (0xFFFFFFFF); /* end of table */

    /* DataTable */
    datatable = here ();
    dw (0xE000); /* INITBYTE */
    dw (0x0008); /* LN_TYPE */
    dw (0x0800); /* NT_RESOURCE */
    dw (0xC000); /* INITLONG */
    dw (0x000A); /* LN_NAME */
    dl (res_name);
    dw (0xE000); /* INITBYTE */
    dw (0x000E); /* LIB_FLAGS */
    dw (0x0600); /* LIBF_SUMUSED | LIBF_CHANGED */
    dw (0xD000); /* INITWORD */
    dw (0x0014); /* LIB_VERSION */
    dw (UAEMAJOR);
    dw (0xD000); /* INITWORD */
    dw (0x0016); /* LIB_REVISION */
    dw (UAEMINOR);
    dw (0xC000); /* INITLONG */
    dw (0x0018); /* LIB_IDSTRING */
    dl (res_id);
    dw (0x0000); /* end of table */

    res_init = here ();
    dl (SIZEOF_LIBRARY + 16); /* size of device base */
    dl (functable);
    dl (datatable);
    dl (initcode);
}