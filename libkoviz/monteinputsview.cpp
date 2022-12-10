#include "monteinputsview.h"
#include <QHeaderView>

MonteInputsView::MonteInputsView(QWidget *parent) :
    QTableView(parent)
{
    setSortingEnabled(true);
    sortByColumn(0,Qt::AscendingOrder);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setTextElideMode(Qt::ElideNone);

    QHeaderView* headerView = horizontalHeader();
    headerView->setTextElideMode(Qt::ElideLeft);
#if QT_VERSION >= 0x050000
    headerView->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
    headerView->setResizeMode(QHeaderView::ResizeToContents);
#endif
    connect(headerView,SIGNAL(sectionClicked(int)),
            this,SLOT(_viewHeaderSectionClicked(int)));

}

void MonteInputsView::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);

    _selectModel = new QItemSelectionModel(model);
    setSelectionModel(_selectModel);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setFocusPolicy(Qt::ClickFocus);
    setTabKeyNavigation(false);
    verticalHeader()->hide();
    connect(_selectModel,
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,
            SLOT(_selectModelCurrentChanged(QModelIndex,QModelIndex)));
}

int MonteInputsView::currentRun()
{
    int runId = -1;
    if ( currentIndex().isValid() ) {
        QModelIndex runIdx = model()->sibling(currentIndex().row(),0,
                                              currentIndex());
        bool ok;
        runId = model()->data(runIdx).toInt(&ok);
        if ( !ok ) {
            runId = -1;
        }
    }
    return runId;
}

void MonteInputsView::_selectModelCurrentChanged(
                     const QModelIndex &curr, const QModelIndex &prev)
{
    Q_UNUSED(prev);

    if ( ! curr.model() ) return ;

#if 0
    QModelIndex runIdx = curr.model()->index(curr.row(),0);
    int runId = curr.model()->data(runIdx).toInt();
    _plotBookView->selectRun(runId);
#endif
}


void MonteInputsView::_viewHeaderSectionClicked(int section)
{
    Q_UNUSED(section);

    QModelIndexList selIdxs = _selectModel->selectedIndexes();
    if ( selIdxs.size() > 0 ) {
        QModelIndex idx = selIdxs.at(section);
        scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}
