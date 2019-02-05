#pragma once

#include <portable_concurrency/future_fwd>

class QImage;
class network_thread;

[[nodiscard]] pc::future<QImage> load_tile(network_thread& nm, int x, int y, int z_level);
