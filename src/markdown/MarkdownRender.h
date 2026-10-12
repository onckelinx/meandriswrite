#pragma once

#include <QColor>
#include <QString>

class QTextDocument;

namespace Markdown {

// Replaces the contents of doc with rendered Markdown (GitHub dialect) and applies the
// preview look. background is the colour the document is shown on; code and table
// colours are derived from it so both light and dark themes work.
void render(QTextDocument *doc, const QString &markdown, const QColor &background);

} // namespace Markdown
