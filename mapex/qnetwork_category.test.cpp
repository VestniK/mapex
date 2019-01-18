#include <QtTest/QtTest>

#include <mapex/qnetwork_category.hpp>

Q_DECLARE_METATYPE(std::errc);

class qnetwork_category_tests : public QObject {
  Q_OBJECT
private:
  bool starts_with(const std::string& str, const std::string& prefix) {
    auto mismatch_pos = std::mismatch(str.begin(), str.end(), prefix.begin(), prefix.end());
    return mismatch_pos.second == prefix.end();
  }

  void generate_all_errors_dataset() {
    QTest::addColumn<QNetworkReply::NetworkError>("error");

    QTest::addRow("NoError") << QNetworkReply::NoError;

    QTest::addRow("ConnectionRefusedError") << QNetworkReply::ConnectionRefusedError;
    QTest::addRow("RemoteHostClosedError") << QNetworkReply::RemoteHostClosedError;
    QTest::addRow("HostNotFoundError") << QNetworkReply::HostNotFoundError;
    QTest::addRow("TimeoutError") << QNetworkReply::TimeoutError;
    QTest::addRow("OperationCanceledError") << QNetworkReply::OperationCanceledError;
    QTest::addRow("SslHandshakeFailedError") << QNetworkReply::SslHandshakeFailedError;
    QTest::addRow("TemporaryNetworkFailureError") << QNetworkReply::TemporaryNetworkFailureError;
    QTest::addRow("NetworkSessionFailedError") << QNetworkReply::NetworkSessionFailedError;
    QTest::addRow("BackgroundRequestNotAllowedError") << QNetworkReply::BackgroundRequestNotAllowedError;
    QTest::addRow("TooManyRedirectsError") << QNetworkReply::TooManyRedirectsError;
    QTest::addRow("InsecureRedirectError") << QNetworkReply::InsecureRedirectError;
    QTest::addRow("UnknownNetworkError") << QNetworkReply::UnknownNetworkError;

    QTest::addRow("ProxyConnectionRefusedError") << QNetworkReply::ProxyConnectionRefusedError;
    QTest::addRow("ProxyConnectionClosedError") << QNetworkReply::ProxyConnectionClosedError;
    QTest::addRow("ProxyNotFoundError") << QNetworkReply::ProxyNotFoundError;
    QTest::addRow("ProxyTimeoutError") << QNetworkReply::ProxyTimeoutError;
    QTest::addRow("ProxyAuthenticationRequiredError") << QNetworkReply::ProxyAuthenticationRequiredError;
    QTest::addRow("UnknownProxyError") << QNetworkReply::UnknownProxyError;

    QTest::addRow("ContentAccessDenied") << QNetworkReply::ContentAccessDenied;
    QTest::addRow("ContentOperationNotPermittedError") << QNetworkReply::ContentOperationNotPermittedError;
    QTest::addRow("ContentNotFoundError") << QNetworkReply::ContentNotFoundError;
    QTest::addRow("AuthenticationRequiredError") << QNetworkReply::AuthenticationRequiredError;
    QTest::addRow("ContentReSendError") << QNetworkReply::ContentReSendError;
    QTest::addRow("ContentConflictError") << QNetworkReply::ContentConflictError;
    QTest::addRow("ContentGoneError") << QNetworkReply::ContentGoneError;
    QTest::addRow("UnknownContentError") << QNetworkReply::UnknownContentError;

    QTest::addRow("ProtocolUnknownError") << QNetworkReply::ProtocolUnknownError;
    QTest::addRow("ProtocolInvalidOperationError") << QNetworkReply::ProtocolInvalidOperationError;
    QTest::addRow("ProtocolFailure") << QNetworkReply::ProtocolFailure;

    QTest::addRow("InternalServerError") << QNetworkReply::InternalServerError;
    QTest::addRow("OperationNotImplementedError") << QNetworkReply::OperationNotImplementedError;
    QTest::addRow("ServiceUnavailableError") << QNetworkReply::ServiceUnavailableError;
    QTest::addRow("UnknownServerError") << QNetworkReply::UnknownServerError;
  }

private slots:
  void NoError_converts_to_zero_error_code() {
    std::error_code ec = QNetworkReply::NoError;
    QVERIFY(!ec);
  }

  void ec_value_equals_to_enum_value_data() { generate_all_errors_dataset(); }
  void ec_value_equals_to_enum_value() {
    QFETCH(QNetworkReply::NetworkError, error);
    std::error_code ec = error;
    QCOMPARE(ec.value(), static_cast<int>(error));
  }

  void ec_category_is_qnetwork_category_data() { generate_all_errors_dataset(); }
  void ec_category_is_qnetwork_category() {
    QFETCH(QNetworkReply::NetworkError, error);
    std::error_code ec = error;
    QCOMPARE(ec.category(), qnetwork_category());
  }

  void ec_message_matches_the_error() {
    std::error_code ec = QNetworkReply::OperationCanceledError;
    QCOMPARE(ec.message(), "the operation was canceled via calls to abort() or close() before it was finished.");
  }

  void ec_message_is_not_unknown_code_message_data() { generate_all_errors_dataset(); }
  void ec_message_is_not_unknown_code_message() {
    QFETCH(QNetworkReply::NetworkError, error);
    std::error_code ec = error;
    QVERIFY(!starts_with(ec.message(), "Unknown QNetworkReply::NetworkError code:"));
  }

  void ec_matches_semantically_equivalent_generic_conditions_data() {
    QTest::addColumn<QNetworkReply::NetworkError>("error");
    QTest::addColumn<std::errc>("generic");

    QTest::addRow("ConnectionRefusedError") << QNetworkReply::ConnectionRefusedError << std::errc::connection_refused;
    QTest::addRow("RemoteHostClosedError") << QNetworkReply::RemoteHostClosedError << std::errc::connection_reset;
    QTest::addRow("TimeoutError") << QNetworkReply::TimeoutError << std::errc::timed_out;
  }
  void ec_matches_semantically_equivalent_generic_conditions() {
    QFETCH(QNetworkReply::NetworkError, error);
    QFETCH(std::errc, generic);
    std::error_code ec = error;
    std::error_condition cond = generic;
    QCOMPARE(ec, cond);
  }

  void ec_matches_same_value_and_category_condition_data() { generate_all_errors_dataset(); }
  void ec_matches_same_value_and_category_condition() {
    QFETCH(QNetworkReply::NetworkError, error);
    std::error_code ec = error;
    std::error_condition cond{ec.value(), ec.category()};
    QCOMPARE(ec, cond);
  }
};

QTEST_MAIN(qnetwork_category_tests)
#include "qnetwork_category.test.moc"
