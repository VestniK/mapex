#pragma once

#include <system_error>

#include <QtNetwork/QNetworkReply>

namespace std {
template<> struct is_error_code_enum<QNetworkReply::NetworkError>: std::true_type {};
}

const std::error_category& qnetwork_category() noexcept;

inline std::error_code make_error_code(QNetworkReply::NetworkError err) noexcept {
  return {static_cast<int>(err), qnetwork_category()};
}
