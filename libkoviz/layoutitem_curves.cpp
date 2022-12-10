#include "layoutitem_curves.h"

CurvesLayoutItem::CurvesLayoutItem(PlotBookModel* bookModel,
                                   const QModelIndex& plotIdx,
                                   QPixmap *pixmap) :
    PaintableLayoutItem(),
    _bookModel(bookModel),
    _plotIdx(plotIdx),
    _pixmap(pixmap)
{
}

CurvesLayoutItem::~CurvesLayoutItem()
{
}

Qt::Orientations CurvesLayoutItem::expandingDirections() const
{
    return (Qt::Horizontal|Qt::Vertical);
}

QRect CurvesLayoutItem::geometry() const
{
    return _rect;
}

bool CurvesLayoutItem::isEmpty() const
{
    return true;
}

QSize CurvesLayoutItem::maximumSize() const
{
    return QSize(0,0);
}

QSize CurvesLayoutItem::minimumSize() const
{
    return QSize(0,0);
}

void CurvesLayoutItem::setGeometry(const QRect &r)
{
    _rect = r;
}

QSize CurvesLayoutItem::sizeHint() const
{
    QSize size(0,0);
    return size;
}

void CurvesLayoutItem::paint(QPainter *painter,
                             const QRect &R, const QRect &RG,
                             const QRect &C, const QRectF &M)
{
    const QRectF RM = mathRect(RG,C,M);
    QTransform T = coordToDotTransform(R,RM);

    painter->save();
    painter->setClipRect(R);

    QModelIndex curvesIdx = _bookModel->getIndex(_plotIdx,"Curves","Plot");
    int nCurves = _bookModel->rowCount(curvesIdx);

    // Print!
    if ( nCurves == 2 ) {
        QString plotPresentation = _bookModel->getDataString(_plotIdx,
                                                     "PlotPresentation","Plot");
        if ( plotPresentation == "compare" ) {
            _printCoplot(T,painter,_plotIdx);
        } else if (plotPresentation == "error" || plotPresentation.isEmpty()) {
            _printErrorplot(T,painter,_plotIdx);
        } else if ( plotPresentation == "error+compare" ) {
            _printErrorplot(T,painter,_plotIdx);
            _printCoplot(T,painter,_plotIdx);
        } else {
            fprintf(stderr,"koviz [bad scoobs]: printCurves() : pres=\"%s\" "
                           "not recognized.\n",
                           plotPresentation.toLatin1().constData());
            exit(-1);
        }
    } else {

        int nElements = 0;
        for ( int i = 0; i < nCurves; ++i ) {
            QModelIndex curveIdx = _bookModel->index(i,0,curvesIdx);
            QPainterPath* path = _bookModel->getPainterPath(curveIdx);
            nElements += path->elementCount();
        }

        if ( nElements > 100000 || nCurves > 64 ) {

            // Use pixmaps to reduce file size
            double rw = R.width()/(double)painter->device()->logicalDpiX();
            double rh = R.height()/(double)painter->device()->logicalDpiY();
            QPixmap nullPixmap(1,1); // used for dpi
            QPainter nullPainter(&nullPixmap);
            int w = qRound(1.8*rw*nullPainter.device()->logicalDpiX());
            int h = qRound(1.8*rh*nullPainter.device()->logicalDpiY());
            QPixmap pixmap(w,h);

            QModelIndex pageIdx = _plotIdx.parent().parent();
            pixmap.fill(_bookModel->pageBackgroundColor(pageIdx));
            QPainter pixmapPainter(&pixmap);
            QPen pen;
            pen.setWidth(0);
            pixmapPainter.setRenderHint(QPainter::Antialiasing);

            QRectF M = _bookModel->getPlotMathRect(_plotIdx);
            double a = w/M.width();
            double b = h/M.height();
            double c = -a*M.x();
            double d = -b*M.y();
            QTransform T( a,    0,
                          0,    b,
                          c,    d);

            for ( int i = 0; i < nCurves; ++i ) {
                QModelIndex curveIdx = _bookModel->index(i,0,curvesIdx);
                QPainterPath* path =_bookModel->getPainterPath(curveIdx);
                if ( path ) {
                    // Line color
                    QColor color(_bookModel->getDataString(curveIdx,
                                                         "CurveColor","Curve"));
                    pen.setColor(color);
                    pixmapPainter.setPen(pen);

                    // Scale transform (e.g. for unit axis scaling)
                    double xs = _bookModel->xScale(curveIdx);
                    double ys = _bookModel->yScale(curveIdx);
                    double xb = _bookModel->xBias(curveIdx);
                    double yb = _bookModel->yBias(curveIdx);
                    QTransform Tscaled(T);
                    Tscaled = Tscaled.scale(xs,ys);
                    Tscaled = Tscaled.translate(xb/xs,yb/ys);
                    pixmapPainter.setTransform(Tscaled);

                    // Line style
                    QString lineStyle = _bookModel->getDataString(curveIdx,
                                                      "CurveLineStyle","Curve");
                    lineStyle = lineStyle.toLower();

                    // Draw curve!
                    if ( lineStyle == "thick_line" ||
                         lineStyle == "x_thick_line" ) {
                        // The transform cannot be used when drawing thick lines
                        QTransform I;
                        pixmapPainter.setTransform(I);
                        double w = pen.widthF();
                        if ( lineStyle == "thick_line" ) {
                            pen.setWidth(5.0);
                        } else if ( lineStyle == "x_thick_line" ) {
                            pen.setWidthF(9.0);
                        } else {
                            fprintf(stderr, "koviz [bad scoobs]: "
                                    "BookView::_paintCurve: bad linestyle\n");
                            exit(-1);
                        }
                        pixmapPainter.setPen(pen);
                        QPointF pLast;
                        for ( int i = 0; i < path->elementCount(); ++i ) {
                            QPainterPath::Element el = path->elementAt(i);
                            QPointF p(el.x,el.y);
                            p = Tscaled.map(p);
                            if  ( i > 0 ) {
                                pixmapPainter.drawLine(pLast,p);
                            }
                            pLast = p;
                        }
                        pen.setWidthF(w);
                        pixmapPainter.setPen(pen);
                        pixmapPainter.setTransform(Tscaled);
                    } else if ( lineStyle == "scatter" ) {
                        QTransform I;
                        pixmapPainter.setTransform(I);
                        double w = pen.widthF();
                        pen.setWidth(3.0);
                        pixmapPainter.setPen(pen);
                        QBrush origBrush = pixmapPainter.brush();
                        QBrush brush(Qt::SolidPattern);
                        brush.setColor(color);
                        pixmapPainter.setBrush(brush);
                        double r = pen.widthF();
                        for ( int i = 0; i < path->elementCount(); ++i ) {
                            QPainterPath::Element el = path->elementAt(i);
                            QPointF p(el.x,el.y);
                            p = Tscaled.map(p);
                            pixmapPainter.drawEllipse(p,r,r);
                        }
                        pen.setWidthF(w);
                        pixmapPainter.setPen(pen);
                        pixmapPainter.setBrush(origBrush);
                        pixmapPainter.setTransform(Tscaled);
                    } else {
                        pixmapPainter.drawPath(*path);
                    }
                }
            }
            QRectF S(pixmap.rect());
            painter->drawPixmap(R,pixmap,S);
        } else {
            _printCoplot(T,painter,_plotIdx);
        }
    }

    // In case of pixmaps, paint grid last
    _paintGrid(painter,R,RG,C,M);


    // Restore the painter state off the painter stack
    painter->restore();

    // Draw legend
    _paintCurvesLegend(R,curvesIdx,painter);
}

void CurvesLayoutItem::_printCoplot(const QTransform& T,
                            QPainter *painter, const QModelIndex &plotIdx)
{
    double start = _bookModel->getDataDouble(QModelIndex(),"StartTime");
    double stop = _bookModel->getDataDouble(QModelIndex(),"StopTime");

    QString plotXScale = _bookModel->getDataString(plotIdx,
                                                   "PlotXScale","Plot");
    QString plotYScale = _bookModel->getDataString(plotIdx,
                                                   "PlotYScale","Plot");
    bool isXLogScale = ( plotXScale == "log" ) ? true : false;
    bool isYLogScale = ( plotYScale == "log" ) ? true : false;

    QList<QPainterPath*> paths;
    QModelIndex curvesIdx = _bookModel->getIndex(plotIdx,"Curves","Plot");
    int rc = _bookModel->rowCount(curvesIdx);
    for ( int i = 0; i < rc; ++i ) {

        QModelIndex curveIdx = _bookModel->index(i,0,curvesIdx);
        QModelIndex curveDataIdx = _bookModel->getDataIndex(curveIdx,
                                                           "CurveData","Curve");
        QVariant v = _bookModel->data(curveDataIdx);
        CurveModel* curveModel =QVariantToPtr<CurveModel>::convert(v);

        if ( curveModel ) {

            double xs = _bookModel->xScale(curveIdx);
            double ys = _bookModel->yScale(curveIdx);
            double xb = _bookModel->xBias(curveIdx);
            double yb = _bookModel->yBias(curveIdx);

            QPainterPath* path = new QPainterPath;
            paths << path;

            curveModel->map();
            ModelIterator* it = curveModel->begin();

            bool isFirst = true;
            while ( !it->isDone() ) {

                if ( it->t() < start || it->t() > stop ) {
                    it->next();
                    continue;
                }

                double x = it->x()*xs+xb;
                double y = it->y()*ys+yb;
                if ( isXLogScale ) x = log10(x);
                if ( isYLogScale ) y = log10(y);
                QPointF p(x,y);
                p = T.map(p);

                if ( isFirst ) {
                    path->moveTo(p.x(),p.y());
                    isFirst = false;
                } else {
                    path->lineTo(p);
                }

                it->next();
            }
            delete it;

            // If curve is flat (constant), label with "Flatline=#"
            QRectF curveBBox = path->boundingRect();
            if ( curveBBox.height() == 0.0 ) {
                it = curveModel->begin();
                double y = it->y()*ys+yb;  // y is constant, so use first point
                delete it;
                QString s;
                s = s.sprintf("%.9g",y);
                QVariant v(s);
                double y2 = v.toDouble();
                double e = qAbs(y-y2);
                if ( e > 1.0e-9 ) {
                    // If %.9g loses too much accuracy, use %lf
                    s = s.sprintf("%.9lf",y);
                }
                s = QString("Flatline=%1").arg(s);
                int h = painter->fontMetrics().height();
                QColor color( _bookModel->getDataString(curveIdx,
                                                        "CurveColor","Curve"));
                QPen pen = painter->pen();
                pen.setColor(color);
                painter->setPen(pen);
                painter->drawText(curveBBox.topLeft()-QPointF(0,h+10),s);
            }

            curveModel->unmap();
        }
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QPen pen = painter->pen();

    double xHeight = painter->fontMetrics().xHeight();
    if ( paths.size() > 20 ) {
        pen.setWidthF(xHeight/11.0);// smaller point size if many curves
    } else if ( paths.size() > 5 && paths.size() < 20 ) {
        pen.setWidthF(xHeight/7.0);
    } else if ( paths.size() < 5 ) {
        pen.setWidthF(xHeight/5.0);
    }
    if (pen.widthF() < 1.0 ) {
        pen.setWidthF(0.0); // cosmetic
    }

    int i = 0;
    foreach ( QPainterPath* path, paths ) {
        QModelIndex curveIdx = _bookModel->index(i,0,curvesIdx);
        QColor color( _bookModel->getDataString(curveIdx,
                                                "CurveColor","Curve"));
        pen.setColor(color);
        QString linestyle =  _bookModel->getDataString(curveIdx,
                                                      "CurveLineStyle","Curve");
        pen.setDashPattern(_bookModel->getLineStylePattern(linestyle));

        // Handle thick_line and x_thick_line styles
        QString style = _bookModel->getDataString(curveIdx,
                                                  "CurveLineStyle","Curve");
        style = style.toLower();
        double penWidthOrig = pen.widthF();
        if ( style == "thick_line" ) {
            if ( pen.widthF() == 0.0 ) {
                pen.setWidth(3.0);
            } else {
                pen.setWidth(pen.widthF()*3.0);
            }
        } else if ( style == "x_thick_line" ) {
            if ( pen.widthF() == 0.0 ) {
                pen.setWidth(5.0);
            } else {
                pen.setWidth(pen.widthF()*5.0);
            }
        } else if ( style == "scatter" ) {
            if ( pen.widthF() == 0.0 ) {
                pen.setWidthF(1.0);
            } else {
                pen.setWidthF(pen.widthF()*1.0);
            }
        }
        painter->setPen(pen);

        if ( style == "scatter" ) {
            QBrush origBrush = painter->brush();
            QBrush brush(Qt::SolidPattern);
            brush.setColor(color);
            painter->setBrush(brush);
            for ( int i = 0; i < path->elementCount(); ++i ) {
                QPainterPath::Element el = path->elementAt(i);
                QPointF p(el.x,el.y);
                double r = pen.widthF();
                painter->drawEllipse(p,r,r); // Qt would not drawPoint for me!
            }
            painter->setBrush(origBrush);
        } else {
            painter->drawPath(*path);
        }
        pen.setWidthF(penWidthOrig);

        // Draw symbols
        QString symbolStyle = _bookModel->getDataString(curveIdx,
                                                   "CurveSymbolStyle", "Curve");
        symbolStyle = symbolStyle.toLower();
        if ( !symbolStyle.isEmpty() && symbolStyle != "none" ) {
            QVector<qreal> pattern;
            pen.setDashPattern(pattern); // plain lines for drawing symbols
            double w = pen.widthF();
            pen.setWidthF(xHeight/11.0);
            painter->setPen(pen);
            QPointF pLast;
            for ( int i = 0; i < path->elementCount(); ++i ) {
                QPainterPath::Element el = path->elementAt(i);
                QPointF p(el.x,el.y);
                if ( i > 0 ) {
                    double r = xHeight*3.0;
                    double x = pLast.x()-r/2.0;
                    double y = pLast.y()-r/2.0;
                    QRectF R(x,y,r,r);
                    if ( R.contains(p) ) {
                        continue;
                    }
                }
                __paintSymbol(p,symbolStyle,painter);

                pLast = p;
            }
            pen.setWidthF(w);
            painter->setPen(pen);
        }
        delete path;
        ++i;
    }
    painter->restore();
}

void CurvesLayoutItem::__paintSymbol(const QPointF &p,
                                     const QString &symbol, QPainter* painter)
{
    QPen origPen = painter->pen();
    QPen pen = painter->pen();
    pen.setStyle(Qt::SolidLine);
    painter->setPen(pen);

    if ( symbol == "circle" ) {
        painter->drawEllipse(p,36,36);
    } else if ( symbol == "thick_circle" ) {
        pen.setWidth(18.0);
        painter->setPen(pen);
        painter->drawEllipse(p,32,32);
    } else if ( symbol == "solid_circle" ) {
        pen.setWidthF(18.0);
        painter->setPen(pen);
        painter->drawEllipse(p,24,24);
        painter->drawEllipse(p,12,12);
    } else if ( symbol == "square" ) {
        double x = p.x()-30.0;
        double y = p.y()-30.0;
        painter->drawRect(QRectF(x,y,60,60));
    } else if ( symbol == "thick_square") {
        pen.setWidthF(16.0);
        painter->setPen(pen);
        double x = p.x()-30.0;
        double y = p.y()-30.0;
        painter->drawRect(QRectF(x,y,60,60));
    } else if ( symbol == "solid_square" ) {
        pen.setWidthF(16.0);
        painter->setPen(pen);
        double x = p.x()-30.0;
        double y = p.y()-30.0;
        painter->drawRect(QRectF(x,y,60,60));
        pen.setWidthF(24.0);
        painter->setPen(pen);
        x = p.x()-12.0;
        y = p.y()-12.0;
        painter->drawRect(QRectF(x,y,24,24));
    } else if ( symbol == "star" ) { // *
        pen.setWidthF(12.0);
        painter->setPen(pen);
        double r = 36.0;
        QPointF a(p.x()+r*cos(18.0*M_PI/180.0),
                  p.y()-r*sin(18.0*M_PI/180.0));
        QPointF b(p.x(),p.y()-r);
        QPointF c(p.x()-r*cos(18.0*M_PI/180.0),
                  p.y()-r*sin(18.0*M_PI/180.0));
        QPointF d(p.x()-r*cos(54.0*M_PI/180.0),
                  p.y()+r*sin(54.0*M_PI/180.0));
        QPointF e(p.x()+r*cos(54.0*M_PI/180.0),
                  p.y()+r*sin(54.0*M_PI/180.0));
        painter->drawLine(p,a);
        painter->drawLine(p,b);
        painter->drawLine(p,c);
        painter->drawLine(p,d);
        painter->drawLine(p,e);
    } else if ( symbol == "xx" ) {
        pen.setWidthF(12.0);
        painter->setPen(pen);
        QPointF a(p.x()+24.0,p.y()+24.0);
        QPointF b(p.x()-24.0,p.y()+24.0);
        QPointF c(p.x()-24.0,p.y()-24.0);
        QPointF d(p.x()+24.0,p.y()-24.0);
        painter->drawLine(p,a);
        painter->drawLine(p,b);
        painter->drawLine(p,c);
        painter->drawLine(p,d);
    } else if ( symbol == "triangle" ) {
        double r = 48.0;
        pen.setJoinStyle(Qt::MiterJoin);
        painter->setPen(pen);
        QPointF a(p.x(),p.y()-r);
        QPointF b(p.x()-r*cos(30.0*M_PI/180.0),
                  p.y()+r*sin(30.0*M_PI/180.0));
        QPointF c(p.x()+r*cos(30.0*M_PI/180.0),
                  p.y()+r*sin(30.0*M_PI/180.0));
        QPolygonF triangle;
        triangle << a << b << c;
        painter->drawConvexPolygon(triangle);
    } else if ( symbol == "thick_triangle" ) {
        pen.setWidthF(24.0);
        pen.setJoinStyle(Qt::MiterJoin);
        painter->setPen(pen);
        double r = 48.0;
        QPointF a(p.x(),p.y()-r);
        QPointF b(p.x()-r*cos(30.0*M_PI/180.0),
                  p.y()+r*sin(30.0*M_PI/180.0));
        QPointF c(p.x()+r*cos(30.0*M_PI/180.0),
                  p.y()+r*sin(30.0*M_PI/180.0));
        QPolygonF triangle;
        triangle << a << b << c;
        painter->drawConvexPolygon(triangle);
    } else if ( symbol == "solid_triangle" ) {
        pen.setWidthF(36.0);
        pen.setJoinStyle(Qt::MiterJoin);
        painter->setPen(pen);
        double r = 36.0;
        QPointF a(p.x(),p.y()-r);
        QPointF b(p.x()-r*cos(30.0*M_PI/180.0),
                  p.y()+r*sin(30.0*M_PI/180.0));
        QPointF c(p.x()+r*cos(30.0*M_PI/180.0),
                  p.y()+r*sin(30.0*M_PI/180.0));
        QPolygonF triangle;
        triangle << a << b << c;
        painter->drawConvexPolygon(triangle);
    } else if ( symbol.startsWith("number_") && symbol.size() == 8 ) {
        QFont origFont = painter->font();
        QBrush origBrush = painter->brush();

        // Calculate bbox to draw text in
        QString number = symbol.right(1); // last char is '0'-'9'
        QFont font = painter->font();
        font.setPointSize(6);
        painter->setFont(font);
        QFontMetrics fm = painter->fontMetrics();
        QRectF bbox(fm.tightBoundingRect(number));
        bbox.moveCenter(p);

        // Draw solid circle around number
        QRectF box(bbox);
        double l = 3.0*qMax(box.width(),box.height())/2.0;
        box.setWidth(l);
        box.setHeight(l);
        box.moveCenter(p);
        QBrush brush(pen.color());
        painter->setBrush(brush);
        painter->drawEllipse(box);

        // Draw number in white in middle of circle
        QPen whitePen("white");
        painter->setPen(whitePen);
        painter->drawText(bbox,Qt::AlignCenter,number);

        painter->setFont(origFont);
        painter->setBrush(origBrush);
    }

    painter->setPen(origPen);
}

void CurvesLayoutItem::_printErrorplot(const QTransform& T,
                                       QPainter *painter,
                                       const QModelIndex &plotIdx)
{
    QModelIndex curvesIdx = _bookModel->getIndex(plotIdx,"Curves","Plot");

    QModelIndex curveIdx0 = _bookModel->index(0,0,curvesIdx);
    QModelIndex curveIdx1 = _bookModel->index(1,0,curvesIdx);

    QVariant v0 = _bookModel->data(_bookModel->
                                   getDataIndex(curveIdx0,"CurveData","Curve"));
    QVariant v1 = _bookModel->data(_bookModel->
                                   getDataIndex(curveIdx1,"CurveData","Curve"));
    CurveModel* c0 = QVariantToPtr<CurveModel>::convert(v0);
    CurveModel* c1 = QVariantToPtr<CurveModel>::convert(v1);

    if ( c0 == 0 || c1 == 0 ) {
        fprintf(stderr,"koviz [bad scoobs]:1: BookView::_printErrorplot()\n");
        exit(-1);
    }

    // Make list of scaled (e.g. unit) data points
    double start = _bookModel->getDataDouble(QModelIndex(),"StartTime");
    double stop = _bookModel->getDataDouble(QModelIndex(),"StopTime");
    double tolerance = _bookModel->getDataDouble(QModelIndex(),
                                                   "TimeMatchTolerance");
    QList<QPointF> pts;
    double k0 = _bookModel->getDataDouble(curveIdx0,"CurveYScale","Curve");
    double k1 = _bookModel->getDataDouble(curveIdx1,"CurveYScale","Curve");
    double ys0 = _bookModel->yScale(curveIdx0);
    double ys1 = (k1/k0)*_bookModel->yScale(curveIdx1);
    c0->map();
    c1->map();
    ModelIterator* i0 = c0->begin();
    ModelIterator* i1 = c1->begin();
    while ( !i0->isDone() && !i1->isDone() ) {
        double t0 = i0->t();
        double t1 = i1->t();
        if ( qAbs(t1-t0) < tolerance ) {
            if ( t0 >= start && t0 <= stop ) {
                double d = ys0*i0->y() - ys1*i1->y();
                pts << QPointF(t0,d);
            }
            i0->next();
            i1->next();
        } else {
            if ( t0 < t1 ) {
                i0->next();
            } else if ( t1 < t0 ) {
                i1->next();
            } else {
                fprintf(stderr,"koviz [bad scoobs]:2: _printErrorplot()\n");
                exit(-1);
            }
        }
    }
    delete i0;
    delete i1;
    c0->unmap();
    c1->unmap();


    // Create path from points
    QPainterPath path;
    if ( !pts.isEmpty() ) {
        bool isFirst = true;
        foreach ( QPointF p, pts ) {
            p = T.map(p);
            if ( isFirst ) {
                isFirst = false;
                path.moveTo(p);
            } else {
                path.lineTo(p);
            }
        }
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Set pen color
    QPen origPen = painter->pen();
    QPen ePen(painter->pen());
    ePen.setWidthF(16.0);
    QRectF curveBBox = path.boundingRect();
    if (curveBBox.height() == 0.0 && !pts.isEmpty() && pts.first().y() == 0.0) {
        // Color green if error plot is flatline zero
        ePen.setColor(_bookModel->flatLineColor());
    } else {
        ePen.setColor(_bookModel->errorLineColor());
    }
    painter->setPen(ePen);

    if ( curveBBox.height() == 0.0 && !pts.isEmpty() ) {
        // If curve is flat (constant), label with "Flatline=#"
        QString yval;
        if ( pts.at(0).y() == 0.0 ) {
            yval = yval.sprintf("Flatline=0.0");
        } else {
            yval = yval.sprintf("Flatline=%g",pts.at(0).y());
        }
        int h = painter->fontMetrics().descent();
        painter->drawText(curveBBox.topLeft()-QPointF(0,h),yval);
    }

    painter->drawPath(path);  // print!

    painter->setPen(origPen);
    painter->restore();
}

void CurvesLayoutItem::_paintGrid(QPainter* painter,
                                  const QRect &R,const QRect &RG,
                                  const QRect &C, const QRectF &M)
{
    // If Grid is off, do not paint grid
    QModelIndex isGridIdx = _bookModel->getDataIndex(_plotIdx,
                                                     "PlotGrid","Plot");
    bool isGrid = _bookModel->data(isGridIdx).toBool();
    if ( !isGrid ) {
        return;
    }

    QString plotXScale = _bookModel->getDataString(_plotIdx,
                                                   "PlotXScale","Plot");
    QString plotYScale = _bookModel->getDataString(_plotIdx,
                                                   "PlotYScale","Plot");
    bool isXLogScale = ( plotXScale == "log" ) ? true : false;
    bool isYLogScale = ( plotYScale == "log" ) ? true : false;

    QList<double> xtics = _bookModel->majorXTics(_plotIdx);
    if ( isXLogScale ) {
        xtics.append(_bookModel->minorXTics(_plotIdx));
    }
    QList<double> ytics = _bookModel->majorYTics(_plotIdx);
    if ( isYLogScale ) {
        ytics.append(_bookModel->minorYTics(_plotIdx));
    }

    QVector<QPointF> vLines;
    QVector<QPointF> hLines;

    const QRectF RM = mathRect(RG,C,M);
    QTransform T = coordToDotTransform(R,RM);

    foreach ( double x, xtics ) {
        vLines << T.map(QPointF(x,RM.top())) << T.map(QPointF(x,RM.bottom()));
    }
    foreach ( double y, ytics ) {
        hLines << T.map(QPointF(RM.left(),y)) << T.map(QPointF(RM.right(),y));
    }

    bool isAntiAliasing = (QPainter::Antialiasing & painter->renderHints()) ;

    // Grid Color
    QModelIndex pageIdx = _bookModel->getIndex(_plotIdx,"Page","Plot");
    QColor color = _bookModel->pageForegroundColor(pageIdx);
    color.setAlpha(40);

    // Pen
    QVector<qreal> dashes;
    qreal space = 4;
    if ( isXLogScale || isYLogScale ) {
        dashes << 1 << 1 ;
    } else {
        dashes << 4 << space ;
    }

    //
    // Draw!
    //
    painter->save();
    painter->setClipRect(R);
    painter->setRenderHint(QPainter::Antialiasing,false);
    QPen origPen = painter->pen();
    QPen pen = painter->pen();
    pen.setColor(color);
    int penWidth = painter->fontMetrics().xHeight()/7;  // 7 arbitrary
    pen.setWidth(penWidth);
    pen.setDashPattern(dashes);
    painter->setPen(pen);
    painter->drawLines(hLines);
    painter->drawLines(vLines);
    painter->setPen(origPen);
    if ( isAntiAliasing ) {
        painter->setRenderHint(QPainter::Antialiasing);
    }
    painter->restore();
}

void CurvesLayoutItem::_paintCurvesLegend(const QRect& R,
                                          const QModelIndex &curvesIdx,
                                          QPainter* painter)
{
    const int maxEntries = 7;

    QString isShowPlotLegend = _bookModel->getDataString(QModelIndex(),
                                                         "IsShowPlotLegend","");

    int nCurves = _bookModel->rowCount(curvesIdx);
    if ( isShowPlotLegend == "no"  ||
        (isShowPlotLegend == "" && (nCurves > maxEntries || nCurves <= 1)) ) {
        return;
    }

    // If all plots on the page have the same legend, PageTitle will show legend
    if (_bookModel->isPlotLegendsSame(curvesIdx.parent().parent().parent())
        && isShowPlotLegend != "yes" ) {
        return;
    }

    QString pres = _bookModel->getDataString(curvesIdx.parent(),
                                             "PlotPresentation","Plot");
    if ( pres == "error" ) {
        return;
    }

    QModelIndex plotIdx = curvesIdx.parent();
    QList<QPen*> pens = _bookModel->legendPens(plotIdx,
                                               painter->paintEngine()->type());
    QStringList symbols = _bookModel->legendSymbols(plotIdx);
    QStringList labels = _bookModel->legendLabels(plotIdx);

    if ( pres == "error+compare" ) {
        QPen* magentaPen = new QPen(_bookModel->errorLineColor());
        pens << magentaPen;
        symbols << "none";
        labels << "error";
    }

    __paintCurvesLegend(R,curvesIdx,pens,symbols,labels,painter);

    // Clean up
    foreach ( QPen* pen, pens ) {
        delete pen;
    }
}

// pens,symbols and labels are ordered/collated foreach legend curve/label
void CurvesLayoutItem::__paintCurvesLegend(const QRect& R,
                                           const QModelIndex &curvesIdx,
                                           const QList<QPen *> &pens,
                                           const QStringList &symbols,
                                           const QStringList &labels,
                                           QPainter* painter)
{
    painter->save();
    QPen origPen = painter->pen();

    int n = pens.size();

    // Width Of Legend Box
    const int fw = painter->fontMetrics().averageCharWidth();
    const int ml = fw; // marginLeft
    const int mr = fw; // marginRight
    const int s = fw;  // spaceBetweenLineAndLabel
    const int l = 4*fw;  // line width , e.g. ~5 for: *-----* Gravity
    int w = 0;
    foreach (QString label, labels ) {
        int labelWidth = painter->fontMetrics().boundingRect(label).width();
        int ww = ml + l + s + labelWidth + mr;
        if ( ww > w ) {
            w = ww;
        }
    }

    // Height Of Legend Box
    const int ls = painter->fontMetrics().lineSpacing();
    const int fh = painter->fontMetrics().height();
    const int v = ls/8;  // vertical space between legend entries (0 works)
    const int mt = fh/4; // marginTop
    const int mb = fh/4; // marginBot
    int sh = 0;
    foreach (QString label, labels ) {
        sh += painter->fontMetrics().boundingRect(label).height();
    }
    int h = (n-1)*v + mt + mb + sh;

    // Legend box top left point
    QString pos = _bookModel->getDataString(QModelIndex(),
                                            "PlotLegendPosition","");
    const int tb = fh/4;    // top/bottom margin
    const int rl = fw/2; // right/left margin
    QPoint legendTopLeft;
    if ( pos == "ne" ) { // The default
        legendTopLeft = QPoint(R.right()-w-rl,R.top()+tb);
    } else if ( pos == "e" ) {
        legendTopLeft = QPoint(R.right()-w-rl,R.height()/2-h/2);
    } else if ( pos == "se" ) {
        legendTopLeft = QPoint(R.right()-w-rl,R.height()-h-tb);
    } else if ( pos == "s" ) {
        legendTopLeft = QPoint(R.width()/2-w/2,R.height()-h-tb);
    } else if ( pos == "sw" ) {
        legendTopLeft = QPoint(R.left()+rl,R.height()-h-tb);
    } else if ( pos == "w" ) {
        legendTopLeft = QPoint(R.left()+rl,R.height()/2-h/2);
    } else if ( pos == "nw" ) {
        legendTopLeft = QPoint(R.left()+rl,R.top()+tb);
    } else if ( pos == "n" ) {
        legendTopLeft = QPoint(R.width()/2-w/2,R.top()+tb);
    } else {
        fprintf(stderr, "koviz [bad scoobs]: "
                        "CurvesLayoutItem::__paintCurvesLegend() has "
                        "bad legend position \"%s\"\n",
                pos.toLatin1().constData());
        exit(-1);
    }

    // Legend box
    QRect LegendBox(legendTopLeft,QSize(w,h));

    // Background color
    QModelIndex pageIdx = curvesIdx.parent().parent().parent();
    QColor bg = _bookModel->pageBackgroundColor(pageIdx);
    bg.setAlpha(190);

    // Draw legend box with semi-transparent background
    painter->setBrush(bg);
    QPen penGray(QColor(120,120,120,255));
    painter->setPen(penGray);
    painter->drawRect(LegendBox);
    painter->setPen(origPen);

    QRect lastBB;
    for ( int i = 0; i < n; ++i ) {

        QString label = labels.at(i);

        // Calc bounding rect (bb) for line and label
        QPoint topLeft;
        if ( i == 0 ) {
            topLeft.setX(legendTopLeft.x()+ml);
            topLeft.setY(legendTopLeft.y()+mt);
        } else {
            topLeft.setX(lastBB.bottomLeft().x());
            topLeft.setY(lastBB.bottomLeft().y()+v);
        }
        QRect bb = painter->fontMetrics().boundingRect(label);
        bb.moveTopLeft(topLeft);
        bb.setWidth(l+s+bb.width());

        // Draw line segment
        QPen* pen = pens.at(i);
        painter->setPen(*pen);
        QPoint p1(bb.left(),bb.center().y());
        QPoint p2(bb.left()+l,bb.center().y());
        painter->drawLine(p1,p2);

        // Draw symbols on line segment endpoints
        QString symbol = symbols.at(i);
        __paintSymbol(p1,symbol,painter);
        __paintSymbol(p2,symbol,painter);

        // Draw label
        QRect labelRect(bb);
        QPoint p(bb.topLeft().x()+l+s, bb.topLeft().y());
        labelRect.moveTopLeft(p);
        painter->drawText(labelRect, Qt::AlignLeft|Qt::AlignVCenter, label);

        lastBB = bb;
    }

    painter->setPen(origPen);
    painter->restore();
}
