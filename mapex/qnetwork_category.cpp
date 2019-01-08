#include <mapex/qnetwork_category.hpp>

const std::error_category& qnetwork_category() noexcept {
  static const struct : std::error_category {
    const char* name() const noexcept override {return "QNetworkReply::NetworkError";}

    std::string message(int cond) const noexcept override {
      switch (static_cast<QNetworkReply::NetworkError>(cond)) {
      case QNetworkReply::NoError:
        return "no error condition.";
      case QNetworkReply::ConnectionRefusedError:
        return "the remote server refused the connection (the server is not accepting requests)";
      case QNetworkReply::RemoteHostClosedError:
        return "the remote server closed the connection prematurely, before the entire reply was received and "
               "processed";
      case QNetworkReply::HostNotFoundError:
        return "the remote host name was not found (invalid hostname)";
      case QNetworkReply::TimeoutError:
        return "the connection to the remote server timed out";
      case QNetworkReply::OperationCanceledError:
        return "the operation was canceled via calls to abort() or close() before it was finished.";
      case QNetworkReply::SslHandshakeFailedError:
        return "the SSL/TLS handshake failed and the encrypted channel could not be established. The sslErrors() "
               "signal should have been emitted.";
      case QNetworkReply::TemporaryNetworkFailureError:
        return "the connection was broken due to disconnection from the network, however the system has initiated "
               "roaming to another access point. The request should be resubmitted and will be processed as soon as "
               "the connection is re-established.";
      case QNetworkReply::NetworkSessionFailedError:
        return "the connection was broken due to disconnection from the network or failure to start the network.";
      case QNetworkReply::BackgroundRequestNotAllowedError:
        return "the background request is not currently allowed due to platform policy.";
      case QNetworkReply::TooManyRedirectsError:
        return "while following redirects, the maximum limit was reached. The limit is by default set to 50 or as set "
               "by QNetworkRequest::setMaxRedirectsAllowed().";
      case QNetworkReply::InsecureRedirectError:
        return "while following redirects, the network access API detected a redirect from a encrypted protocol "
               "(https) to an unencrypted one (http). ";
      case QNetworkReply::UnknownNetworkError:
        return "an unknown network-related error was detected";
      case QNetworkReply::ProxyConnectionRefusedError:
        return "the connection to the proxy server was refused (the proxy server is not accepting requests)";
      case QNetworkReply::ProxyConnectionClosedError:
        return "the proxy server closed the connection prematurely, before the entire reply was received and processed";
      case QNetworkReply::ProxyNotFoundError:
        return "the proxy host name was not found (invalid proxy hostname)";
      case QNetworkReply::ProxyTimeoutError:
        return "the connection to the proxy timed out or the proxy did not reply in time to the request sent";
      case QNetworkReply::ProxyAuthenticationRequiredError:
        return "the proxy requires authentication in order to honour the request but did not accept any credentials "
               "offered (if any)";
      case QNetworkReply::UnknownProxyError:
        return "an unknown proxy-related error was detected";
      case QNetworkReply::ContentAccessDenied:
        return "the access to the remote content was denied (similar to HTTP error 403)";
      case QNetworkReply::ContentOperationNotPermittedError:
        return "the operation requested on the remote content is not permitted";
      case QNetworkReply::ContentNotFoundError:
        return "the remote content was not found at the server (similar to HTTP error 404)";
      case QNetworkReply::AuthenticationRequiredError:
        return "the remote server requires authentication to serve the content but the credentials provided were not "
               "accepted (if any)";
      case QNetworkReply::ContentReSendError:
        return "the request needed to be sent again, but this failed for example because the upload data could not be "
               "read a second time.";
      case QNetworkReply::ContentConflictError:
        return "the request could not be completed due to a conflict with the current state of the resource.";
      case QNetworkReply::ContentGoneError:
        return "the requested resource is no longer available at the server.";
      case QNetworkReply::UnknownContentError:
        return "an unknown error related to the remote content was detected";
      case QNetworkReply::ProtocolUnknownError:
        return "the Network Access API cannot honor the request because the protocol is not known";
      case QNetworkReply::ProtocolInvalidOperationError:
        return "the requested operation is invalid for this protocol";
      case QNetworkReply::ProtocolFailure:
        return "a breakdown in protocol was detected (parsing error, invalid or unexpected responses, etc.)";
      case QNetworkReply::InternalServerError:
        return "the server encountered an unexpected condition which prevented it from fulfilling the request.";
      case QNetworkReply::OperationNotImplementedError:
        return "the server does not support the functionality required to fulfill the request.";
      case QNetworkReply::ServiceUnavailableError:
        return "the server is unable to handle the request at this time.";
      case QNetworkReply::UnknownServerError:
        return "an unknown error related to the server response was detected";
      }
      return "Unknown QNetworkReply::NetworkError code: " + std::to_string(cond);
    }

    bool equivalent(int code, const std::error_condition& cond) const noexcept override {
      return
        (code == QNetworkReply::ConnectionRefusedError && cond == std::errc::connection_refused) ||
        (code == QNetworkReply::RemoteHostClosedError && cond == std::errc::connection_reset) ||
        (code == QNetworkReply::TimeoutError && cond == std::errc::timed_out);
    }
  } inst;
  return inst;
}
