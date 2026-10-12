#include "editor/FindBar.h"

#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

FindBar::FindBar(QWidget *parent) : QWidget(parent), m_edit(new QLineEdit)
{
    m_edit->setClearButtonEnabled(true);
    m_edit->setPlaceholderText(tr("Find"));

    auto *previous = new QToolButton;
    previous->setText(tr("Previous"));
    auto *next = new QToolButton;
    next->setText(tr("Next"));
    auto *close = new QToolButton;
    close->setText(QStringLiteral("×"));
    close->setAutoRaise(true);
    close->setToolTip(tr("Close (Esc)"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->addWidget(new QLabel(tr("Find:")));
    layout->addWidget(m_edit, 1);
    layout->addWidget(previous);
    layout->addWidget(next);
    layout->addWidget(close);

    connect(previous, &QToolButton::clicked, this, &FindBar::findPrevious);
    connect(next, &QToolButton::clicked, this, &FindBar::findNext);
    connect(close, &QToolButton::clicked, this, [this] {
        hide();
        emit closed();
    });
    connect(m_edit, &QLineEdit::textEdited, this, &FindBar::searchEdited);
    connect(m_edit, &QLineEdit::returnPressed, this, [this] {
        if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
            emit findPrevious();
        else
            emit findNext();
    });
}

void FindBar::activate(const QString &initialText)
{
    if (!initialText.isEmpty())
        m_edit->setText(initialText);
    setNotFound(false);
    show();
    m_edit->setFocus();
    m_edit->selectAll();
}

QString FindBar::text() const
{
    return m_edit->text();
}

void FindBar::setNotFound(bool notFound)
{
    m_edit->setStyleSheet(notFound ? QStringLiteral("QLineEdit { background: #f6d5d5; color: #000; }") : QString());
}

void FindBar::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
        emit closed();
        return;
    }
    QWidget::keyPressEvent(event);
}
