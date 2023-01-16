#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "os9types.h"
#include "xspress3.h"
#include "xspress3test.h"
#include "errors.h"


int generate_scope_data()
{
	XSP3ScopeModule *mod;
	mh_com *mod_head;
	u_int16_t type_lang=0, attr_rev = 0;
	int err;
	char *name="xsp3_scope";
	int stream;
	int card;
	u_int16_t *p;
	int i;

	if ((err=_os_link(&name, &mod_head, (void **)&mod, &type_lang, &attr_rev)) != 0)
	{
		printf("Cannot link to xspress scope module '%s', err=%d\n", name, err);
		return -1;
	}

	mprintf(1, "SCOPE-TEST_PAT\n");
	for (card=0;card<mod->head.num_cards; card++)
	{
		if ((test_gen_scope & 0xFFFF) == TEST_GEN_SCOPE_MIXED)
			mod->head.card[card].src_sel = XSP3_GSCOPE_SRC_SEL0(XSP3_SCOPE_SEL0_DIGITAL);
		else if  ((test_gen_scope & 0xFFFF) == TEST_GEN_SCOPE_ALL_DIG)
			mod->head.card[card].src_sel = XSP3_GSCOPE_SRC_SEL0(XSP3_SCOPE_SEL0_ALL_RESETS);
		else 
			mod->head.card[card].src_sel = XSP3_GSCOPE_SRC_SEL0(XSP3_SCOPE_SEL0_INP);
		mod->head.card[card].chan_sel = 0;
		for (stream=0; stream<6; stream++)
		{
			mod->head.card[card].chan_sel |= XSP3_GSCOPE_CHAN_SEL(stream,(stream+card)%9);
		}
	}

	for (card=0;card<mod->head.num_cards; card++)
	{
		mprintf(1, "Building incrementing pattern for %d cards, num_t=%d\n", mod->head.num_cards, mod->head.num_t);

		for (stream=0; stream<6; stream++)
		{
			p = xsp3_scope_mod_get_ptr(mod, card, stream);
			for (i=0;i<mod->head.num_t; i++)
			{
				*p = i/(stream+1);
				p += 3;
			}
		}
	}
	return 0;
}
