#include "print/Printing.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QPrintDialog>
#include <QPrinter>
#include <QTextDocument>
#include <QUrl>

#include <memory>

#include "markdown/MarkdownRender.h"

namespace Printing {

namespace {

std::unique_ptr<QTextDocument> buildTextDocument(const Document &document)
{
    auto doc = std::make_unique<QTextDocument>();
    if (!document.filePath.isEmpty())
        doc->setBaseUrl(QUrl::fromLocalFile(QFileInfo(document.filePath).absolutePath() + QLatin1Char('/')));

    if (document.markdown) {
        Markdown::render(doc.get(), document.text, Qt::white); // paper is white, whatever the theme
    } else {
        doc->setDefaultFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        doc->setPlainText(document.text);
    }
    return doc;
}

} // namespace

void print(QWidget *parent, const Document &document)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName(document.title);
    QPrintDialog dialog(&printer, parent);
    if (dialog.exec() != QDialog::Accepted)
        return;
    buildTextDocument(document)->print(&printer);
}

void exportPdf(QWidget *parent, const Document &document)
{
    QString suggested;
    if (document.filePath.isEmpty()) {
        suggested = QDir::home().filePath(document.title + QStringLiteral(".pdf"));
    } else {
        const QFileInfo info(document.filePath);
        suggested = info.dir().filePath(info.completeBaseName() + QStringLiteral(".pdf"));
    }

    QString path = QFileDialog::getSaveFileName(parent, QObject::tr("Export as PDF"), suggested,
                                                QObject::tr("PDF (*.pdf)"));
    if (path.isEmpty())
        return;
    if (!path.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive))
        path += QStringLiteral(".pdf");

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setDocName(document.title);
    buildTextDocument(document)->print(&printer);
}

} // namespace Printing
