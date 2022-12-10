#ifndef DPTREEWIDGET_H
#define DPTREEWIDGET_H

#include <QGridLayout>
#include <QLineEdit>
#include <QTreeView>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QDir>
#include <QHash>
#include <QProgressDialog>
#include "dp.h"
#include "dpfilterproxymodel.h"
#include "bookmodel.h"
#include "utils.h"
#include "monteinputsview.h"
#include "programmodel.h"

// This class introduced to fix Qt bug:
// https://codereview.qt-project.org/#/c/65171/3
class DPTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit DPTreeView(QWidget *parent = 0);

protected:
    virtual void currentChanged(const QModelIndex &current,
                                const QModelIndex &previous);
    virtual void selectionChanged(const QItemSelection &selected,
                                  const QItemSelection &deselected);
};

// Data Products TreeView with Search Box

class DPTreeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DPTreeWidget(const QString& timeName,
                          const QString& dpDirName,
                          const QStringList& dpFiles,
                          QStandardItemModel* dpVarsModel,
                          const QStringList& runDirs,
                          PlotBookModel* bookModel,
                          QItemSelectionModel*  bookSelectModel,
                          MonteInputsView* monteInputsView,
                          bool isShowTables,
                          const QStringList& unitOverrides,
                          QWidget *parent = 0);
    ~DPTreeWidget();
    
signals:
    
public slots:

private:
    int _idNum;
    QString _timeName;
    QString _dpDirName;
    QStringList _dpFiles;
    QStandardItemModel* _dpVarsModel;
    QDir* _dir;
    QStringList _runDirs;
    PlotBookModel* _bookModel;
    QItemSelectionModel*  _bookSelectModel;
    MonteInputsView* _monteInputsView;
    bool _isShowTables;
    QStringList _unitOverrides;
    QGridLayout* _gridLayout ;
    QLineEdit* _searchBox;
    DPTreeView* _dpTreeView ;
    DPFilterProxyModel* _dpFilterModel;
    QFileSystemModel* _dpModel ;
    QModelIndex _dpModelRootIdx;
    QList<ProgramModel*> _programModels;

    void _setupModel();
    void _createDP(const QString& dpfile);
    void _createDPPages(const QString& dpfile);
    void _createDPTables(const QString& dpfile);
    QStandardItem* _addChild(QStandardItem* parentItem,
                   const QString& childTitle,
                   const QVariant &childValue=QVariant());
    CurveModel* _addCurve(QStandardItem* curvesItem, DPCurve* dpcurve,
                          DPProgram *dpprogram,
                          int runId,
                          const QString &defaultColor,
                          const QString &defaultLineStyle);
    bool _isDP(const QString& fp);
    QString _descrPlotTitle(DPPlot* plot);

    static QString _err_string;
    static QTextStream _err_stream;

public slots:
    void clearSelection();

private slots:
    void _searchBoxTextChanged(const QString &rx);
     void _dpTreeViewCurrentChanged(const QModelIndex &currIdx,
                                    const QModelIndex &prevIdx);

};

#endif // DPTREEVIEW_H
