#include <QApplication>
#include <QDateTime>
#include <fstream>
#include "euRunApplication.h"
#include "euRun.hh"
#include "eudaq/OptionParser.hh"
#include "Colours.hh"
#include "config.h" // for version symbols


euRunApplication::euRunApplication(int& argc, char** argv) :
    QApplication(argc, argv) {}

bool euRunApplication::notify(QObject* receiver, QEvent* event) {
    bool done = true;
    try {
        done = QApplication::notify(receiver, event);
    } catch (const std::exception& ex) {
      //get current date
      QDate date = QDate::currentDate();
      std::cout << date.toString().toStdString() << ": euRun GUI caught (and ignored) exception: " << ex.what() << std::endl;
        
    } catch (...) {
      //get current date
      QDate date = QDate::currentDate();
      std::cout << date.toString().toStdString() << ": euRun GUI caught (and ignored) unspecified exception " << std::endl;
    }
    return done;
} 


int main(int argc, char ** argv) {
  euRunApplication app(argc, argv);
  eudaq::OptionParser op("EUDAQ Run Control", PACKAGE_VERSION, "A Qt version of the Run Control");
  eudaq::Option<std::string>  addr(op, "a", "listen-address", "tcp://44000", "address",  "The address on which to listen for connections");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
  eudaq::Option<int>             x(op, "x", "left",   0, "pos");
  eudaq::Option<int>             y(op, "y", "top",    0, "pos");
  eudaq::Option<int>             w(op, "w", "width",  150, "pos");
  eudaq::Option<int>             h(op, "g", "height", 200, "pos", "The initial position of the window");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    QRect rect(x.Value(), y.Value(), w.Value(), h.Value());
    RunControlGUI gui(addr.Value(), rect);
    gui.show();
    return euRunApplication::exec();
  } catch (...) {
    std::cout << "euRun exception handler" << std::endl;
    std::ostringstream err;
    int result = op.HandleMainException(err);
    if (!err.str().empty()) {
      QMessageBox::warning(nullptr, "Exception", err.str().c_str());
    }
    return result;
  }
}

RunConnectionDelegate::RunConnectionDelegate(RunControlModel * model) : m_model(model) {}

void RunConnectionDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const {
  //std::cout << __FUNCTION__ << std::endl;
  //painter->save();
  int level = m_model->GetLevel(index);
  painter->fillRect(option.rect, QBrush(level_colours[level]));
  QItemDelegate::paint(painter, option, index);
  //painter->restore();
}

RunControlGUI::RunControlGUI(const std::string & listenaddress, QRect geom, QWidget *parent, Qt::WindowFlags flags):
  QMainWindow(parent, flags),
  eudaq::RunControl(listenaddress),
  m_delegate(&m_run),
  m_prevcoinc(0),
  m_prevtime(0.0),
  m_runstarttime(0.0),
  m_filebytes(0),
  dostatus(false),
  m_event_number(0) {

  status_labels.push_back(std::make_pair<std::string, std::string>("RUN", "Run Number"));
  status_labels.push_back(std::make_pair<std::string, std::string>("FILEBYTES", "File Size"));
  status_labels.push_back(std::make_pair<std::string, std::string>("EVENT", "Events Built"));
  status_labels.push_back(std::make_pair<std::string, std::string>("TUSTAT", "TU Status"));
  status_labels.push_back(std::make_pair<std::string, std::string>("COINCCOUNT", "Coincidence Count"));
  status_labels.push_back(std::make_pair<std::string, std::string>("COINCRATE", "Coincidence Rate"));
  status_labels.push_back(std::make_pair<std::string, std::string>("BEAMCURR", "Beam Current"));

  scaler_labels.push_back(std::make_pair<std::string, std::string>("SC1", "Plane 1"));
  scaler_labels.push_back(std::make_pair<std::string, std::string>("SC5", "Plane 5"));
  scaler_labels.push_back(std::make_pair<std::string, std::string>("SC2", "Plane 2"));
  scaler_labels.push_back(std::make_pair<std::string, std::string>("SC6", "Plane 6"));
  scaler_labels.push_back(std::make_pair<std::string, std::string>("SC3", "Plane 3"));
  scaler_labels.push_back(std::make_pair<std::string, std::string>("SC7", "Plane 7"));
  scaler_labels.push_back(std::make_pair<std::string, std::string>("SC4", "Plane 4"));
  scaler_labels.push_back(std::make_pair<std::string, std::string>("SC8", "Plane 8"));
  scaler_labels.push_back(std::make_pair<std::string, std::string>("SC0", "Scintillator"));
  scaler_labels.push_back(std::make_pair<std::string, std::string>("SC9", "DUT"));

  setupUi(this);

  if (grpStatus->layout() == nullptr) { grpStatus->setLayout(new QGridLayout(grpStatus)); }
  auto * layout = dynamic_cast<QGridLayout *>(grpStatus->layout());
  int row(0), col(0);
  for (const auto & i_status: status_labels) {
    QLabel * lblname = new QLabel(grpStatus);
    lblname->setObjectName(QString("lbl_st_") + i_status.first.c_str());
    lblname->setText((i_status.second + ":").c_str());
    QLabel * lblvalue = new QLabel(grpStatus);
    lblvalue->setObjectName(QString("txt_st_") + i_status.first.c_str());
    layout->addWidget(lblname, row / 2, (col % 2) * 2);
    layout->addWidget(lblvalue, row++ / 2, (col++ % 2) * 2 + 1);
    m_status[i_status.first] = lblvalue;
  }

  if (scalerStatus->layout() == nullptr) { scalerStatus->setLayout(new QGridLayout(scalerStatus)); }
  auto * scaler_layout = dynamic_cast<QGridLayout *>(scalerStatus->layout());
  row = 0; col = 0;
  for (const auto & i_scaler: scaler_labels) {
    QLabel * lblname = new QLabel(scalerStatus);
    lblname->setObjectName(QString("lbl_st_") + i_scaler.first.c_str());
    lblname->setText((i_scaler.second + ":").c_str());
    QLabel * lblvalue = new QLabel(scalerStatus);
    lblvalue->setObjectName(QString("txt_st_") + i_scaler.first.c_str());
    scaler_layout->addWidget(lblname, row / 2, (col % 2) * 2);
    scaler_layout->addWidget(lblvalue, row++ / 2, (col++ % 2) * 2 + 1);
    m_status[i_scaler.first] = lblvalue;
  }
  //end added


  viewConn->setModel(&m_run);
  viewConn->setItemDelegate(&m_delegate);
  QDir dir("../conf/", "*.conf");
  for (size_t i = 0; i < dir.count(); ++i) {
    QString item = dir[i];
    if (item.toStdString().find("converter") != std::string::npos) {  //exclude the converter config files
      continue;
    }
    item.chop(5);
    cmbConfig->addItem(item);
  }
  cmbConfig->setEditText(cmbConfig->itemText(0));
  QSettings flux_settings("../conf/flux.ini", QSettings::IniFormat);
  std::vector<int> fluxes;
  for (const auto & iflux: flux_settings.childGroups()) {
    fluxes.push_back(iflux.toInt());
  }
  std::sort(fluxes.begin(), fluxes.end());
  for (auto iflux: fluxes) {
    cmbFlux->addItem(QString(std::to_string(iflux).c_str()));
  }
  cmbFlux->setEditText(QString(std::to_string(fluxes.at(0)).c_str()));
  QSize fsize = frameGeometry().size();
  if (geom.x() == -1) { geom.setX(x()); }
  geom.setY(geom.y() == -1 ? y() : geom.y() + MAGIC_NUMBER);
  if (geom.width() == -1) { geom.setWidth(fsize.width()); }
  if (geom.height() == -1) { geom.setHeight(fsize.height()); }
  //else geom.setHeight(geom.height() - MAGIC_NUMBER);
  move(geom.topLeft());
  resize(geom.size());
  connect(this, SIGNAL(StatusChanged(const QString &, const QString &)), this, SLOT(ChangeStatus(const QString &, const QString &)));
  connect(&m_statustimer, SIGNAL(timeout()), this, SLOT(timer()));
  connect(this,SIGNAL(btnLogSetStatus(bool)),this, SLOT(btnLogSetStatusSlot(bool)));
  connect(this, SIGNAL(SetState(int)),this,SLOT(SetStateSlot(int)));
  m_statustimer.start(500);
//  txtGeoID->setText(QString::number(eudaq::ReadFromFile(GEOID_FILE, 0U)));
//  txtGeoID->installEventFilter(this);
  setWindowIcon(QIcon("../images/Icon_euRun.png"));
  setWindowTitle("ETH/PSI Run Control based on eudaq " PACKAGE_VERSION);
}


void RunControlGUI::OnReceive(const eudaq::ConnectionInfo & id, std::shared_ptr<eudaq::Status> status) {
  static bool registered = false;
  if (!registered) {
    qRegisterMetaType<QModelIndex>("QModelIndex");
    registered = true;}

  if (id.GetType() == "DataCollector") {
    m_event_number = from_string(status->GetTag("EVENT"), uint32_t(0));
    m_filebytes = from_string(status->GetTag("FILEBYTES"), 0LL);
    EmitStatus("EVENT", status->GetTag("EVENT"));
    EmitStatus("FILEBYTES", to_bytes(status->GetTag("FILEBYTES")));} 
  

  else if (id.GetType() == "Producer" && id.GetName() == "TU"){
    EmitStatus("TUSTAT", status->GetTag("STATUS"));
    std::string beam_current = status->GetTag("BEAM_CURR");
    EmitStatus("BEAMCURR", (beam_current.empty() ? "" : QString("%1 µA").arg(std::stof(beam_current), 3, 'f', 1).toStdString()));
    EmitStatus("COINCCOUNT", status->GetTag("COINC_COUNT"));

    //update Scaler Status
    std::string scalers;
    for (int i = 0; i < 10; ++i) {
      std::string s = status->GetTag("SCALER" + to_string(i));
      if (s.empty()) { break; }
      std::string s_name = "SC"+to_string(i);
      EmitStatus(s_name.c_str(), s);
      }


    int c_counts = from_string(status->GetTag("COINC_COUNT"), -1);
    double last_ts = from_string(status->GetTag("LASTTIME"), 0.0);
    double ts = from_string(status->GetTag("TIMESTAMP"), 0.0);
    if (c_counts > 0) { m_runstarttime = ts; } //start timing
    double coinc_rate = (c_counts-m_prevcoinc)/(ts-last_ts);
    m_prevcoinc = c_counts;
    if (coinc_rate > 1){ //not to display too small numbers
      EmitStatus("COINCRATE", to_string(coinc_rate)+ " Hz");
    }else{
      EmitStatus("COINCRATE", "0 Hz");
    }

  }//end if producer=tu

    m_run.SetStatus(id, *status);
    
} //end method



void RunControlGUI::OnConnect(const eudaq::ConnectionInfo & id) {
  static bool registered = false;
  if (!registered) {
    qRegisterMetaType<QModelIndex>("QModelIndex");
    registered = true;
  }
  //QMessageBox::information(this, "EUDAQ Run Control",
  //                         "This will reset all connected Producers etc.");
  m_run.newconnection(id);
  if (id.GetType() == "DataCollector") {
    EmitStatus("RUN", "(" + to_string(m_runnumber) + ")");
    SetState(ST_NONE);
  }
  if (id.GetType() == "LogCollector") {
     btnLogSetStatus(true);
  }
}

void RunControlGUI::EmitStatus(const char * name, const std::string & val) {
  if (val.empty()) { return; }
  emit StatusChanged(name, val.c_str());
}


//bool RunControlGUI::eventFilter(QObject *object, QEvent *event) {
//  if (object == txtGeoID && event->type() == QEvent::MouseButtonDblClick) {
//    int oldid = txtGeoID->text().toInt();
//    bool ok = false;
//    int newid = QInputDialog::getInt(this, "Increment GeoID to:", "value", oldid+1, 0, 2147483647, 1, &ok);
//    if (ok) {
//      txtGeoID->setText(QString::number(newid));
//      eudaq::WriteToFile(GEOID_FILE, newid);
//    }
//    return true;
//  }
//  return false;
//}