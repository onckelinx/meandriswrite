#include <QImage>
#include <QMimeData>
#include <QtTest>

#include "clipboard/ClipboardMarkdown.h"
#include "files/TextFile.h"
#include "integration/DesktopIntegration.h"

namespace {

QString readAll(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

void writeAll(const QString &path, const QString &text)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(text.toUtf8());
}

} // namespace

class CoreTest : public QObject {
    Q_OBJECT

    QTemporaryDir m_home; // stands in for ~/.local/share and ~/.config in the integration tests

private slots:
    void initTestCase()
    {
        QVERIFY(m_home.isValid());
        qputenv("XDG_DATA_HOME", m_home.filePath(QStringLiteral("share")).toUtf8());
        qputenv("XDG_CONFIG_HOME", m_home.filePath(QStringLiteral("config")).toUtf8());
        qputenv("XDG_CURRENT_DESKTOP", "KDE");
        qputenv("APPIMAGE", "/opt/my apps/mw\"1.AppImage");
    }

    void textFileRoundTrip_data()
    {
        QTest::addColumn<QByteArray>("bytes");
        QTest::newRow("lf") << QByteArray("one\ntwo\n");
        QTest::newRow("crlf") << QByteArray("one\r\ntwo\r\n");
        QTest::newRow("bom") << QByteArray("\xEF\xBB\xBFone\r\ntwo");
        QTest::newRow("utf8") << QByteArray("caf\xC3\xA9 \xE2\x80\x94 \xC2\xA0nbsp\n");
        QTest::newRow("empty") << QByteArray();
    }

    void textFileRoundTrip()
    {
        QFETCH(QByteArray, bytes);
        TextFileFormat format;
        const QString text = TextFile::decode(bytes, &format);
        QVERIFY(!text.contains(QLatin1Char('\r')));
        QCOMPARE(TextFile::encode(text, format), bytes);
    }

    void latin1Fallback()
    {
        QCOMPARE(TextFile::decode(QByteArray("\xE9t\xE9"), nullptr), QStringLiteral("été"));
    }

    void markdownPaths()
    {
        QVERIFY(TextFile::isMarkdownPath(QStringLiteral("/a/README.MD")));
        QVERIFY(TextFile::isMarkdownPath(QStringLiteral("notes.markdown")));
        QVERIFY(!TextFile::isMarkdownPath(QStringLiteral("notes.txt")));
    }

    void looksLikeMarkdown_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<bool>("expected");
        QTest::newRow("heading") << QStringLiteral("# Title\n\nText") << true;
        QTest::newRow("fence") << QStringLiteral("Look:\n```\ncode\n```\n") << true;
        QTest::newRow("table") << QStringLiteral("| a | b |\n|---|---|\n| 1 | 2 |\n") << true;
        QTest::newRow("bold+code") << QStringLiteral("Use **this** with `that`.") << true;
        QTest::newRow("list+link") << QStringLiteral("- see [docs](https://x.y)\n") << true;
        QTest::newRow("prose") << QStringLiteral("Dear Bob,\nThanks for the update.\n") << false;
        QTest::newRow("list only") << QStringLiteral("- one\n- two\n") << false;
        QTest::newRow("hashtag") << QStringLiteral("#notaheading here") << false;
    }

    void looksLikeMarkdown()
    {
        QFETCH(QString, text);
        QFETCH(bool, expected);
        QCOMPARE(ClipboardMarkdown::looksLikeMarkdown(text), expected);
    }

    void htmlToMarkdown()
    {
        const QString markdown = ClipboardMarkdown::htmlToMarkdown(QStringLiteral(
            "<h1>Title</h1>"
            "<p>Some <b>bold</b>, <i>italic</i> and <code>code</code>.</p>"
            "<ul><li>one</li><li>two</li></ul>"
            "<ol><li>first</li><li>second</li></ol>"
            "<pre>x = 1\ny = 2</pre>"
            "<p><a href=\"https://example.com\">link</a></p>"
            "<table><tr><th>A</th><th>B</th></tr><tr><td>1</td><td>2</td></tr></table>"));
        qInfo().noquote() << "\n" << markdown;

        QVERIFY(markdown.contains(QStringLiteral("# Title")));
        QVERIFY(markdown.contains(QStringLiteral("**bold**")));
        QVERIFY(markdown.contains(QStringLiteral("*italic*")));
        QVERIFY(markdown.contains(QStringLiteral("`code`")));
        QVERIFY(markdown.contains(QStringLiteral("- one")));
        QVERIFY(markdown.contains(QRegularExpression(QStringLiteral(R"(1\. +first)")))); // Qt writes "1.  first"
        QVERIFY(markdown.contains(QStringLiteral("```\nx = 1\ny = 2\n```")));
        QVERIFY(markdown.contains(QStringLiteral("[link](https://example.com)")));
        QVERIFY(markdown.contains(QStringLiteral("|A")) || markdown.contains(QStringLiteral("| A")));
    }

    void clipboardPrefersMarkdownText()
    {
        QMimeData mime;
        mime.setText(QStringLiteral("# Hi\r\n\r\nThere"));
        mime.setHtml(QStringLiteral("<h1>Hi</h1><p>There</p>"));
        QCOMPARE(ClipboardMarkdown::fromMimeData(&mime), QStringLiteral("# Hi\n\nThere"));
    }

    void clipboardConvertsHtml()
    {
        QMimeData mime;
        mime.setText(QStringLiteral("Hi there"));
        mime.setHtml(QStringLiteral("<p>Hi <b>there</b></p>"));
        QCOMPARE(ClipboardMarkdown::fromMimeData(&mime), QStringLiteral("Hi **there**\n"));
    }

    void clipboardKeepsPlainText()
    {
        QMimeData mime;
        mime.setText(QStringLiteral("just text"));
        QCOMPARE(ClipboardMarkdown::fromMimeData(&mime), QStringLiteral("just text"));
    }

    void desktopEntryQuotesExec()
    {
        const QString entry = DesktopIntegration::desktopEntry(QStringLiteral("/opt/my apps/mw\"1.AppImage"));
        QVERIFY(entry.contains(QStringLiteral("\nExec=\"/opt/my apps/mw\\\"1.AppImage\" %F\n")));
        QVERIFY(entry.contains(QStringLiteral("Name=Meandris Write")));
        QVERIFY(entry.contains(QStringLiteral("MimeType=text/plain;text/markdown;")));
    }

    void menuEntry()
    {
#ifndef Q_OS_LINUX
        QSKIP("Linux desktop integration; elsewhere it would write into the real user folders.");
#endif
        QCOMPARE(DesktopIntegration::installKind(), DesktopIntegration::InstallKind::AppImage);
        const QString entry = m_home.filePath(QStringLiteral("share/applications/meandriswrite.desktop"));
        const QString icon = m_home.filePath(QStringLiteral("share/icons/hicolor/256x256/apps/meandriswrite.png"));
        QVERIFY(!DesktopIntegration::isInMenu());
        QVERIFY(DesktopIntegration::setInMenu(true, nullptr));
        QVERIFY(DesktopIntegration::isInMenu());
        QCOMPARE(QImage(icon).size(), QSize(256, 256));

        // Moving the AppImage updates the menu entry on the next start.
        qputenv("APPIMAGE", "/home/me/Apps/mw.AppImage");
        DesktopIntegration::refreshMenuEntry();
        QVERIFY(readAll(entry).contains(QStringLiteral("Exec=\"/home/me/Apps/mw.AppImage\" %F")));

        QVERIFY(DesktopIntegration::setInMenu(false, nullptr));
        QVERIFY(!DesktopIntegration::isInMenu());
        QVERIFY(!QFileInfo::exists(icon));
    }

    void defaultForMarkdown()
    {
#ifndef Q_OS_LINUX
        QSKIP("Linux desktop integration; elsewhere it would write into the real user folders.");
#endif
        const QString shared = m_home.filePath(QStringLiteral("config/mimeapps.list"));
        const QString kde = m_home.filePath(QStringLiteral("config/kde-mimeapps.list"));
        writeAll(shared, QStringLiteral("[Added Associations]\ntext/markdown=kate.desktop;\n\n"
                                        "[Default Applications]\ntext/plain=kate.desktop;\n"));
        writeAll(kde, QStringLiteral("[Default Applications]\ntext/markdown=org.kde.kate.desktop;\n"));
        QVERIFY(!DesktopIntegration::isDefaultForMarkdown());

        QVERIFY(DesktopIntegration::setDefaultForMarkdown(true, nullptr));
        QVERIFY(DesktopIntegration::isDefaultForMarkdown());
        QCOMPARE(readAll(kde), QStringLiteral("[Default Applications]\ntext/markdown=meandriswrite.desktop;\n"));
        QCOMPARE(readAll(shared), QStringLiteral("[Added Associations]\ntext/markdown=kate.desktop;\n\n"
                                                 "[Default Applications]\ntext/plain=kate.desktop;\n"
                                                 "text/markdown=meandriswrite.desktop;\n"));

        // Turning it off brings back the previous default.
        QVERIFY(DesktopIntegration::setDefaultForMarkdown(false, nullptr));
        QVERIFY(!DesktopIntegration::isDefaultForMarkdown());
        QCOMPARE(readAll(kde), QStringLiteral("[Default Applications]\ntext/markdown=org.kde.kate.desktop;\n"));
        QVERIFY(readAll(shared).contains(QStringLiteral("[Default Applications]\ntext/plain=kate.desktop;\n"
                                                        "text/markdown=org.kde.kate.desktop;\n")));
    }

    void defaultForMarkdownWithoutPrevious()
    {
#ifndef Q_OS_LINUX
        QSKIP("Linux desktop integration; elsewhere it would write into the real user folders.");
#endif
        QFile::remove(m_home.filePath(QStringLiteral("config/kde-mimeapps.list")));
        const QString shared = m_home.filePath(QStringLiteral("config/mimeapps.list"));
        QFile::remove(shared);
        QVERIFY(DesktopIntegration::setDefaultForMarkdown(true, nullptr));
        QCOMPARE(readAll(shared), QStringLiteral("[Default Applications]\ntext/markdown=meandriswrite.desktop;\n"));
        QVERIFY(DesktopIntegration::setDefaultForMarkdown(false, nullptr));
        QCOMPARE(readAll(shared), QStringLiteral("[Default Applications]\n"));
    }
};

QTEST_MAIN(CoreTest)
#include "core_test.moc"
