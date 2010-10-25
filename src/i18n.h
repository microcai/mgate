/*
 * i18n.h
 *
 *  Created on: 2010-4-13
 *      Author: cai
 */

#ifndef I18N_H_
#define I18N_H_

#define N_(x) (x)

#ifdef HAVE_GETTEXT
#include <locale.h>
#include <libintl.h>
#define _(x) gettext(x)
#else
#define _(x) N_(x)
#endif

#endif /* I18N_H_ */
