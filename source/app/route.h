#ifndef PSS_APP_ROUTE_H
#define PSS_APP_ROUTE_H

#include "app/model.h"

/* Which screen follows S0. */

typedef enum {
    ROUTE_GALLERY,    /* S2 */
    ROUTE_ONBOARDING, /* S1 */
    ROUTE_E01,        /* error screens, by their code */
    ROUTE_E02,
    ROUTE_E03,
    ROUTE_E04, /* after a purchase that failed with nothing changed */
    ROUTE_E05, /* after a purchase that failed and could not be undone */
    ROUTE_E06,
} Route;

/* After the model was loaded. E-02 is not here: it comes from the boot step before the model. */
Route route_after_load(ModelStatus status);

/* What the result of a purchase leads to (S5). */
typedef enum {
    PURCHASE_SUCCESS,     /* S6 */
    PURCHASE_FAILED,      /* E-04, nothing was changed */
    PURCHASE_UNDO_FAILED, /* E-05, the journal is kept */
} PurchaseOutcome;

PurchaseOutcome route_after_purchase(TxnResult result);

#endif
