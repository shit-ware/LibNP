// ==========================================================
// RepZ project
// 
// Component: offline
// Sub-component: libnp
// Purpose: Enables offline game playe
//
// Initial author: iain17
// Started: 2014-12-24
// ==========================================================

#include "StdInc.h"
#include "NPOffline.h"

LIBNP_API void LIBNP_CALL NP_GoOffline() {
	g_np.offline = true;
}