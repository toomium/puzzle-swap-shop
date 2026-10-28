#include "app/route.h"

Route route_after_load(ModelStatus status)
{
    switch (status) {
    case MODEL_READY:
        return ROUTE_GALLERY;
    case MODEL_NEED_ORIGINAL:
        return ROUTE_ONBOARDING;
    case MODEL_READ_ONLY:
        return ROUTE_E03;
    case MODEL_INTERRUPTED:
        return ROUTE_E06;
    case MODEL_ERR_READ:
    default:
        return ROUTE_E01;
    }
}

PurchaseOutcome route_after_purchase(TxnResult result)
{
    switch (result) {
    case TXN_OK:
        return PURCHASE_SUCCESS;
    case TXN_ERR_ROLLBACK:
        return PURCHASE_UNDO_FAILED;
    default:
        /* Invalid data, read, backup, journal and write errors that were undone, refusals, stopped
         * runs. */
        return PURCHASE_FAILED;
    }
}
