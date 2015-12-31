/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "searchedit.h"

#include "registry/searchquery.h"
#include "ui/icons.h"

#include <QCompleter>
#include <QKeyEvent>
#include <QLabel>
#include <QTreeView>

SearchEdit::SearchEdit(QWidget *parent) :
    QLineEdit(parent),
    m_prefixCompleter(new QCompleter(this)),
    m_completionLabel(new QLabel(this)),
    m_searchLabel(new QLabel(this))
{
    m_completionLabel->setObjectName(QStringLiteral("completer"));
    m_completionLabel->setStyleSheet(QStringLiteral("QLabel#completer { color: gray; }"));
    m_completionLabel->setFont(font());

    displaySearchIcon();

    connect(this, &SearchEdit::textChanged, this, &SearchEdit::displayCompletions);
}

SearchEdit::~SearchEdit()
{
}

/**
 * @brief SearchEdit::displaySearchIcon
 * Displays the search icon for the UI.
 */
void SearchEdit::displaySearchIcon()
{
    QStyleOptionFrameV2 option;
    initStyleOption(&option);

    QSize searchLabelSize(10, 10);
    QFont searchLabelFont("Ionicons", 10);
    searchLabelFont.setStyleStrategy(QFont::PreferAntialias);

    m_searchLabel->setFont(searchLabelFont);
    m_searchLabel->resize(searchLabelSize);
    m_searchLabel->setText(Icons::Search);
    m_searchLabel->setStyleSheet("QLabel { color: #333; }");

    QRect searchLabelGeometry(
                8,
                (height() - searchLabelSize.height()) / 2 + 1,
                searchLabelSize.width(),
                searchLabelSize.height());

    m_searchLabel->setGeometry(searchLabelGeometry);

}

void SearchEdit::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);
    displaySearchIcon();
}

void SearchEdit::setTreeView(QTreeView *view)
{
    m_treeView = view;
    m_focusing = false;
}

/**
 * @brief SearchEdit::setCompletions
 * Sets up the prefix autocompleter.
 * @param completions The list of completions to use.
 */
void SearchEdit::setCompletions(const QStringList &completions)
{
    m_prefixCompleter = std::unique_ptr<QCompleter>(new QCompleter(completions, this));
    m_prefixCompleter->setCompletionMode(QCompleter::InlineCompletion);
    m_prefixCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_prefixCompleter->setWidget(this);
}

/**
 * @brief SearchEdit::clearQuery
 * Clear input with consideration to docset filters.
 * Keeps the docset filter for the first Escape press.
 *
 * Cases:
 * 1. query|      - Cleared to |
 * 2. ruby:|      - Cleared to |
 * 2. ruby:query| - Cleared to ruby:|
 */
void SearchEdit::clearQuery()
{
    setText(text().left(queryStart()));
}

/**
 * @brief SearchEdit::selectQuery
 * Selects the query with consideration to docset filters.
 *
 * Cases:
 * 1. query|      - Selects |query|
 * 2. ruby:|      - Selects |ruby:|
 * 3. ruby:query| - Selects ruby:|query|
 */
void SearchEdit::selectQuery()
{
    setSelection(queryStart(), text().size());
}

bool SearchEdit::event(QEvent *event)
{
    if (event->type() != QEvent::KeyPress)
        return QLineEdit::event(event);

    // Tab key cannot be overriden in keyPressEvent()
    if (reinterpret_cast<QKeyEvent *>(event)->key() != Qt::Key_Tab)
        return QLineEdit::event(event);

    const QString completed = currentCompletion(text());
    if (!completed.isEmpty())
        setText(completed);

    return true;
}

void SearchEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    emit focusIn();

    // Do not change default behaviour when focused with mouse
    if (event->reason() == Qt::MouseFocusReason)
        return;

    selectQuery();
    m_focusing = true;
}

void SearchEdit::focusOutEvent(QFocusEvent *event)
{
    QLineEdit::focusOutEvent(event);
    emit focusOut();
}

void SearchEdit::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        clearQuery();
        event->accept();
        break;
    case Qt::Key_Return:
        emit m_treeView->activated(m_treeView->selectionModel()->currentIndex());
        event->accept();
        break;
    case Qt::Key_Down:
    case Qt::Key_Up: {
        const QModelIndex index = m_treeView->currentIndex();
        const int nextRow = index.row() + (event->key() == Qt::Key_Down ? 1 : -1);
        if (nextRow >= 0 && nextRow < m_treeView->model()->rowCount()) {
            const QModelIndex sibling = index.sibling(nextRow, 0);
            m_treeView->setCurrentIndex(sibling);
        }
        event->accept();
        break;
    }
    default:
        QLineEdit::keyPressEvent(event);
        break;
    }
}

void SearchEdit::mousePressEvent(QMouseEvent *event)
{
    // Let the focusInEvent code deal with initial selection on focus.
    if (!m_focusing)
        QLineEdit::mousePressEvent(event);

    m_focusing = false;
}

/**
 * @brief SearchEdit::displayCompletions
 * Displays the most relevant completion inline of the search edit.
 * Displaying of completion should not affect the `text()` value.
 *
 * @param searchEditText New text of the SearchEdit view.
 */
void SearchEdit::displayCompletions(const QString &searchEditText)
{
    QStyleOptionFrameV2 option;
    initStyleOption(&option);

    QMargins margins = textMargins();

    QRect rect = style()->subElementRect(QStyle::SE_LineEditContents, &option, this);
    const int textWidth = fontMetrics().width(searchEditText);

    int minLB = qMax(0, -fontMetrics().minLeftBearing());
    rect.setX(rect.x() + margins.left() + 2 + minLB);

    m_prefixCompleter->setCompletionPrefix(text());

    const QString completed = currentCompletion(searchEditText).mid(searchEditText.size());
    const QSize labelSize(fontMetrics().width(completed), size().height());

    m_completionLabel->setMinimumSize(labelSize);
    m_completionLabel->move(rect.left() + textWidth, 0);
    m_completionLabel->setText(completed);
}

/**
 * @brief SearchEdit::currentCompletion
 * The most relevant completion for a particular text.
 * @param text Text to complete.
 */
QString SearchEdit::currentCompletion(const QString &text) const
{
    if (text.isEmpty() || !m_prefixCompleter)
        return QString();

    return m_prefixCompleter->currentCompletion();
}

/**
 * @brief SearchEdit::queryStart Returns index at which the query starts.
 *
 * Cases:
 * 1. query|      - Returns 0
 * 2. ruby:|      - Returns 0
 * 3. ruby:query| - Returns 5
 */
int SearchEdit::queryStart() const
{
    const Zeal::SearchQuery currentQuery = Zeal::SearchQuery::fromString(text());
    return currentQuery.query().isEmpty() ? 0 : currentQuery.keywordPrefixSize();
}
