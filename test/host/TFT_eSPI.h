#ifndef HOST_TFT_ESPI_H
#define HOST_TFT_ESPI_H

// DisplayManager owns a TFT_eSPI; tests replace DisplayManager's methods with
// recording fakes, so the driver itself only needs to exist as a type.
class TFT_eSPI {};

#endif
