#include <QFileInfo>
#include "datamodel.h"
#include "datamodel_trick.h"
#include "datamodel_csv.h"
#include "datamodel_mot.h"

DataModel *DataModel::createDataModel(const QStringList &timeNames,
                                      const QString &fileName)
{
    DataModel* dataModel = 0;
    QFileInfo fi(fileName);
    if ( fi.suffix() == "trk") {
        dataModel = new TrickModel(timeNames,fileName);
    } else if ( fi.suffix() == "csv" ) {
        dataModel = new CsvModel(timeNames,fileName);
    } else if ( fi.suffix() == "mot" ) {
        dataModel = new MotModel(timeNames,fileName);
    } else {
        fprintf(stderr,"koviz [error]: DataModel::createDataModel() cannot "
                       "handle file=\"%s\"\n",fileName.toLatin1().constData());
        exit(-1);
    }

    return dataModel;
}
