#pragma once

#include <QString>

class QWidget;

// Printing and PDF export. Markdown prints as the rendered preview, other text as plain text.
namespace Printing {

struct Document {
    QString text;
    bool markdown = false;
    QString filePath; // empty for unsaved documents
    QString title;
};

void print(QWidget *parent, const Document &document);
void exportPdf(QWidget *parent, const Document &document);

} // namespace Printing
