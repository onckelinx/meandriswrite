#pragma once

#include <QWidget>

class QLineEdit;

// Plain-text find bar shown at the bottom of the window (no regex, case-insensitive).
class FindBar : public QWidget {
    Q_OBJECT

public:
    explicit FindBar(QWidget *parent = nullptr);

    // Shows the bar, optionally prefilled, and focuses the search field.
    void activate(const QString &initialText);
    QString text() const;
    void setNotFound(bool notFound);

signals:
    void findNext();
    void findPrevious();
    void searchEdited();
    void closed();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QLineEdit *m_edit;
};
