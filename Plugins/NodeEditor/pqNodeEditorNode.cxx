/*=========================================================================

  Program:   ParaView
  Plugin:    NodeEditor

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  ParaViewPluginsNodeEditor - BSD 3-Clause License - Copyright (C) 2021 Jonas Lukasczyk

  See the Copyright.txt file provided
  with ParaViewPluginsNodeEditor for license information.
-------------------------------------------------------------------------*/

#include "pqNodeEditorNode.h"

#include "pqNodeEditorLabel.h"
#include "pqNodeEditorPort.h"
#include "pqNodeEditorUtils.h"
#include "pqNodeEditorTimingsWidget.h"

#include <pqDataRepresentation.h>
#include <pqOutputPort.h>
#include <pqPipelineFilter.h>
#include <pqPipelineSource.h>
#include <pqProxyWidget.h>
#include <pqServerManagerModel.h>

#include <vtkSMPropertyGroup.h>
#include <vtkSMProxy.h>

#include <QBrush>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QVBoxLayout>

// The pqDoubleLineEdit.h file is only included to handle the issue that the
// simplified notation rendering of pqDoubleLineEdit widgets is currently not
// working correctly in QT Graphics View Framework and therefore needs to be
// explicitly disabled.
#include <pqDoubleLineEdit.h>

// ----------------------------------------------------------------------------
pqNodeEditorNode::Verbosity pqNodeEditorNode::DefaultNodeVerbosity{
  pqNodeEditorNode::Verbosity::NORMAL
};

// ----------------------------------------------------------------------------
pqNodeEditorNode::pqNodeEditorNode(pqProxy* prx, QGraphicsItem* parent)
  : QGraphicsItem(parent)
  , proxy(prx)
  , proxyProperties(new pqProxyWidget(prx->getProxy()))
  , widgetContainer(new QWidget)
  , label(new pqNodeEditorLabel("", this))
{
  this->setZValue(pqNodeEditorUtils::CONSTS::NODE_LAYER);
  this->setFlags({ GraphicsItemFlag::ItemSendsGeometryChanges, GraphicsItemFlag::ItemIsMovable,
    QGraphicsItem::ItemIsSelectable });
  this->setCacheMode(CacheMode::DeviceCoordinateCache);
  this->setCursor(Qt::ArrowCursor);
  this->setObjectName(QString("node") + this->proxy->getSMName());

  // compute headline height
  if (auto* proxyAsSource = dynamic_cast<pqPipelineSource*>(this->proxy))
  {
    int maxNPorts = proxyAsSource->getNumberOfOutputPorts();
    if (auto* proxyAsFilter = dynamic_cast<pqPipelineFilter*>(proxy))
    {
      maxNPorts = std::max(maxNPorts, proxyAsFilter->getNumberOfInputPorts());
    }
    this->headlineHeight = pqNodeEditorUtils::CONSTS::PORT_HEIGHT * maxNPorts +
      pqNodeEditorUtils::CONSTS::PORT_PADDING * (maxNPorts + 1);
  }

  // init label
  {
    this->label->setObjectName("nodeLabel");
    this->label->setCursor(Qt::PointingHandCursor);

    QFont font;
    font.setBold(true);
    font.setPointSize(pqNodeEditorUtils::CONSTS::NODE_FONT_SIZE);
    this->label->setFont(font);

    // This function retrieves the name of the linked proxy and places it in the
    // middle of the node. If necessary the label is scaled to fit inside the
    // node. The function is connected to the nameChanged event.
    auto updateNodeLabel = [this]() {
      this->label->setPlainText(this->proxy->getSMName());
      this->label->setScale(1.0);

      const auto br = this->label->boundingRect();
      const auto nodeWidthToLabelWidthRatio = pqNodeEditorUtils::CONSTS::NODE_WIDTH / br.width();

      // if label width larger than node width resize label
      if (nodeWidthToLabelWidthRatio < 1.0)
      {
        this->label->setScale(nodeWidthToLabelWidthRatio);
      }

      this->label->setPos(
        0.5 * (pqNodeEditorUtils::CONSTS::NODE_WIDTH - br.width() * this->label->scale()), 1);
    };
    QObject::connect(this->proxy, &pqProxy::nameChanged, this->label, updateNodeLabel);

    updateNodeLabel();
    this->labelHeight = this->label->boundingRect().height();
    this->headlineHeight += labelHeight + 3;
  }

  // create a widget container for property and display widgets
  {
    this->widgetContainer->setObjectName("nodeContainer");
    this->widgetContainer->setMinimumWidth(pqNodeEditorUtils::CONSTS::NODE_WIDTH);
    this->widgetContainer->setMaximumWidth(pqNodeEditorUtils::CONSTS::NODE_WIDTH);

    // install resize event filter
    this->widgetContainer->installEventFilter(pqNodeEditorUtils::createInterceptor(
      this->widgetContainer, [this](QObject* /*object*/, QEvent* event) {
        if (event->type() == QEvent::LayoutRequest)
        {
          this->updateSize();
        }
        return false;
      }));

    // initialize property widgets container
    auto containerLayout = new QVBoxLayout;
    this->widgetContainer->setLayout(containerLayout);

    auto graphicsProxyWidget = new QGraphicsProxyWidget(this);
    graphicsProxyWidget->setObjectName("graphicsProxyWidget");
    graphicsProxyWidget->setWidget(this->widgetContainer);
    graphicsProxyWidget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    graphicsProxyWidget->setPos(QPointF(0, this->headlineHeight));

    this->proxyProperties->setObjectName("proxyPropertiesWidget");
    this->proxyProperties->updatePanel();

    // Disable the simplified notation rendering for pqDoubleLineEdit widgets.
    for (auto element : this->proxyProperties->findChildren<pqDoubleLineEdit*>())
    {
      element->setAlwaysUseFullPrecision(true);
    }

    containerLayout->addWidget(this->proxyProperties);
    if (dynamic_cast<pqPipelineSource*>(this->proxy) != NULL ||
	    dynamic_cast<pqPipelineFilter*>(this->proxy) != NULL )
    {
      this->timings = new pqNodeEditorTimingsWidget(this->widgetContainer, this->proxy->getProxy()->GetGlobalID());
      this->timings->setObjectName("timingsWidget");
      // this->timings->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
      this->timings->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
      containerLayout->addWidget(this->timings);
    }

    this->updateSize();
  }

  this->setVerbosity(pqNodeEditorNode::DefaultNodeVerbosity);
  this->updateSize();
}

// ----------------------------------------------------------------------------
pqNodeEditorNode::~pqNodeEditorNode() = default;

// ----------------------------------------------------------------------------
int pqNodeEditorNode::updateSize()
{
  this->prepareGeometryChange();

  this->widgetContainer->resize(this->widgetContainer->layout()->sizeHint());
  Q_EMIT this->nodeResized();

  return 1;
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::setOutlineStyle(OutlineStyle style)
{
  this->outlineStyle = style;
  this->setZValue(style == OutlineStyle::NORMAL ? pqNodeEditorUtils::CONSTS::NODE_LAYER
                                                : pqNodeEditorUtils::CONSTS::NODE_LAYER + 1);
  this->update(this->boundingRect());
}


// ----------------------------------------------------------------------------
void pqNodeEditorNode::setNodeActive(bool active)
{
  this->nodeActive = active;
  this->updateZValue();
  this->update(this->boundingRect());
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::setNodeState(NodeState style)
{
  this->nodeState = style;
  this->update(this->boundingRect());
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::setVerbosity(Verbosity v)
{
  this->verbosity = v;
  switch (this->verbosity)
  {
    // The string "%%%..." is used to filter out every widget that does not contains such string as
    // a tag. Since we're pretty sure no one will ever name or a document a property with such a
    // string this is ok.
    case Verbosity::EMPTY:
      this->proxyProperties->filterWidgets(false, "%%%%%%%%%%%%%%");
      this->widgetContainer->hide();
      this->updateSize();
      break;
    case Verbosity::NORMAL:
      this->proxyProperties->filterWidgets(false);
      this->widgetContainer->show();
      break;
    case Verbosity::ADVANCED:
      this->proxyProperties->filterWidgets(true);
      this->widgetContainer->show();
      break;
    default:
      break;
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::incrementVerbosity()
{
  this->setVerbosity(static_cast<Verbosity>((static_cast<int>(this->verbosity) + 1) % 3));
}

// ----------------------------------------------------------------------------
QVariant pqNodeEditorNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (change == GraphicsItemChange::ItemPositionHasChanged)
  {
    Q_EMIT this->nodeMoved();
  }

  if (change == GraphicsItemChange::ItemSelectedHasChanged)
  {
    this->updateZValue();
  }

  return QGraphicsItem::itemChange(change, value);
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::updateZValue()
{
  this->setZValue(pqNodeEditorUtils::CONSTS::NODE_LAYER + static_cast<int>(this->isSelected() * 2) +
    static_cast<int>(this->isNodeActive()));
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::updateTimings()
{
  if (this->timings)
  {
    this->timings->updateTimings();
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::toggleTimings(int state)
{
  if (this->timings)
  {
    if (state && !this->widgetContainer->isVisible()){
      this->widgetContainer->show();
    }
    else if (!state && this->widgetContainer->isVisible() && this->verbosity == Verbosity::EMPTY)
    {
      this->widgetContainer->hide();
    }
    
    this->timings->setVisible(static_cast<bool>(state));
  }
}

// ----------------------------------------------------------------------------
QRectF pqNodeEditorNode::boundingRect() const
{
  const auto& border = pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  const double height = this->headlineHeight +
    (this->widgetContainer->isVisible() ? this->widgetContainer->height() : 0.0);
  return QRectF(0, 0, pqNodeEditorUtils::CONSTS::NODE_WIDTH, height)
    .adjusted(-border, -border, border, border);
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  QPen pen;
  QBrush brush;
  this->setupPaintTools(pen, brush);

  QPainterPath path;
  // Make sure the whole node is redrawn to avoid artefacts
  constexpr double borderOffset = 0.5 * pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH;
  const QRectF br =
    this->boundingRect().adjusted(borderOffset, borderOffset, -borderOffset, -borderOffset);
  path.addRoundedRect(
    br, pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH, pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH);

  // QPen pen;
  pen.setWidth(pqNodeEditorUtils::CONSTS::NODE_BORDER_WIDTH);
  switch (this->outlineStyle)
  {
    case OutlineStyle::NORMAL:
      pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_CONSTRAST);
      break;
    case OutlineStyle::SELECTED_FILTER:
      pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_HIGHLIGHT);
      break;
    case OutlineStyle::SELECTED_VIEW:
      pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_BASE_ORANGE);
      break;
    case OutlineStyle::LOOP:
      pen.setBrush(pqNodeEditorUtils::CONSTS::COLOR_DULL_GREEN);
      break;
    default:
      break;
  }

  painter->setPen(pen);
  painter->fillPath(path, brush);
  painter->drawPath(path);
}

// ----------------------------------------------------------------------------
QString pqNodeEditorNode::getNodeKey() const
{
  return "node." + this->proxy->getSMGroup() + "." + this->proxy->getSMName();
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::importLayout(const QSettings& settings)
{
  const QString nodeName = this->getNodeKey();

  if (auto verbos = pqNodeEditorUtils::safeGetValue<int>(settings, nodeName + ".verbosity"))
  {
    this->setVerbosity(static_cast<Verbosity>(verbos.Value));
  }
  if (auto transfo = pqNodeEditorUtils::safeGetValue<QTransform>(settings, nodeName + ".transform"))
  {
    this->setTransform(transfo.Value);
  }
  if (auto pos = pqNodeEditorUtils::safeGetValue<QPointF>(settings, nodeName + ".pos"))
  {
    this->setPos(pos.Value);
  }
}

// ----------------------------------------------------------------------------
void pqNodeEditorNode::exportLayout(QSettings& settings)
{
  const QString nodeName = this->getNodeKey();

  auto exportProperty = [&](const char* name, QVariant value) {
    settings.setValue(nodeName + name, value);
  };

  exportProperty(".verbosity", static_cast<int>(this->verbosity));

  exportProperty(".transform", this->transform());

  exportProperty(".pos", this->pos());
}
