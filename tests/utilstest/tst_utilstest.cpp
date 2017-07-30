#include <QtTest/QTest>
#include "utils.h"

class Q_DECL_HIDDEN UtilsTest : public QObject {
	Q_OBJECT
public:
	using QByteArrayPair = QPair<QByteArray, QByteArray>;

private Q_SLOTS:

	void test_parseNV_data()
	{
		QTest::addColumn<QByteArray>("input");
		QTest::addColumn<QList<QPair<QByteArray, QByteArray> > >("res_expected");
		QTest::addColumn<bool>("ret_expected");

		QTest::newRow("std")
			<< QByteArray("\013\002SERVER_PORT80\013\016SERVER_ADDR199.170.183.42")
			<< (
					QList<QPair<QByteArray, QByteArray> >()
						<< qMakePair(QByteArray("SERVER_PORT"), QByteArray("80"))
						<< qMakePair(QByteArray("SERVER_ADDR"), QByteArray("199.170.183.42"))
			   )
			<< true
		;

		QTest::newRow("largesize")
			<< QByteArray("\x80\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0ESERVER_ADDR199.170.183.42", sizeof("\x80\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0ESERVER_ADDR199.170.183.42")-1)
			<< (
					QList<QPair<QByteArray, QByteArray> >()
						<< qMakePair(QByteArray("SERVER_PORT"), QByteArray("80"))
						<< qMakePair(QByteArray("SERVER_ADDR"), QByteArray("199.170.183.42"))
			   )
			<< true
		;

		QTest::newRow("bad1")
			<< QByteArray("\x88\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0ESERVER_ADDR199.170.183.42", sizeof("\x88\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0ESERVER_ADDR199.170.183.42")-1)
			<< (
					QList<QPair<QByteArray, QByteArray> >()
			   )
			<< false
		;

		QTest::newRow("bad2")
			<< QByteArray("\x80\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0FSERVER_ADDR199.170.183.42", sizeof("\x80\x00\x00\x0B\x02SERVER_PORT80\x0B\x80\x00\x00\x0FSERVER_ADDR199.170.183.42")-1)
			<< (
					QList<QPair<QByteArray, QByteArray> >()
						<< qMakePair(QByteArray("SERVER_PORT"), QByteArray("80"))
			   )
			<< false
		;
	}

	void test_parseNV()
	{
		QFETCH(QByteArray, input);
		QFETCH(QList<QByteArrayPair>, res_expected);
		QFETCH(bool, ret_expected);

		QList<QByteArrayPair> res_actual;
		bool ret_actual = FastCGI::LowLevel::parseNV(input, res_actual);

		QCOMPARE(ret_actual, ret_expected);
		QCOMPARE(res_actual, res_expected);
	}

	void test_falttenNV_data()
	{
		QTest::addColumn<QList<QPair<QByteArray, QByteArray> > >("input");
		QTest::addColumn<QByteArray>("res_expected");

		QTest::newRow("std")
			<< (
					QList<QPair<QByteArray, QByteArray> >()
						<< qMakePair(QByteArray("SERVER_PORT"), QByteArray("80"))
						<< qMakePair(QByteArray("SERVER_ADDR"), QByteArray("199.170.183.42"))
			   )
			<< QByteArray("\013\002SERVER_PORT80\013\016SERVER_ADDR199.170.183.42")
		;

		QByteArray one_x128(128, '1');
		QByteArray two_x128(128, '2');
		QTest::newRow("large")
			<< (
					QList<QPair<QByteArray, QByteArray> >()
						<< qMakePair(one_x128, two_x128)
			   )
			<< QByteArray("\x80\x00\x00\x80\x80\x00\x00\x80", 8) + one_x128 + two_x128
		;
	}

	void test_falttenNV()
	{
		QFETCH(QList<QByteArrayPair>, input);
		QFETCH(QByteArray, res_expected);

		QByteArray res_actual = FastCGI::LowLevel::flattenNV(input);

		QCOMPARE(res_actual, res_expected);
	}
};

QTEST_GUILESS_MAIN(UtilsTest)

#include "tst_utilstest.moc"
