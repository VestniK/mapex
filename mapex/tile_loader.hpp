#pragma once

#include <portable_concurrency/future_fwd>

class QNetworkAccessManager;
class QImage;

[[nodiscard]] pc::future<QImage> load_tile(QNetworkAccessManager& nm, int x, int y, int z_level);
