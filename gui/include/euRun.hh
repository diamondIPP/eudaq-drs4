#include "ui_euRun.h"
#include "RunControlModel.hh"
#include "eudaq/RunControl.hh"
#include "eudaq/Utils.hh"
#include "../../build/gui/ui_euRun.h"
#include <QMainWindow>
#include <QMessageBox>
#include <QCloseEvent>
#include <QItemDelegate>
#include <QDir>
#include <QPainter>
#include <QTimer>
#include <QInputDialog>
#include <QSettings>

using eudaq::to_string;
using eudaq::from_string;

inline std::string to_bytes(const std::string & val) {
  if (val == "") return "";
  uint64_t n = from_string(val, 0ULL);
  const char * suff[] = { " B", " kB", " MB", " GB", " TB" };
  const int numsuff = sizeof suff / sizeof *suff;
  int mult = 0;
  while (n/1024 >= 10 && mult < numsuff-1) {
    n /= 1024;
    mult++;
  }
  return to_string(n) + suff[mult];
}

// To make Qt behave on OSX (to be checked on other OSes)
#define MAGIC_NUMBER 22

class RunConnectionDelegate : public QItemDelegate {
  public:
    RunConnectionDelegate(RunControlModel * model);
  private:
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    RunControlModel * m_model;
};

class RunControlGUI : public QMainWindow, public Ui::wndRun, public eudaq::RunControl {
  Q_OBJECT
  public:
    RunControlGUI(const std::string & listenaddress,
        QRect geom,
        QWidget *parent = 0,
        Qt::WindowFlags flags = 0);
  private:
    enum state_t { ST_NONE, ST_READY, ST_RUNNING };
    virtual void OnConnect(const eudaq::ConnectionInfo & id);
    virtual void OnDisconnect(const eudaq::ConnectionInfo & id) { m_run.disconnected(id); }
    virtual void OnReceive(const eudaq::ConnectionInfo & id, std::shared_ptr<eudaq::Status> status);
    void EmitStatus(const char * name, const std::string & val);

    void closeEvent(QCloseEvent * event) {
      if (m_run.rowCount() > 0 &&
          QMessageBox::question(this, "Quitting", "Terminate all connections and quit?",
            QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
        event->ignore();
      } else {
        Terminate();
	eudaq::mSleep(500);
        event->accept();
      }
    }
//    bool eventFilter(QObject *object, QEvent *event);
    private slots:


		void SetStateSlot(int state) {
			btnConfig->setEnabled(state != ST_RUNNING);
			btnStart->setEnabled(state == ST_READY);
			btnStop->setEnabled(state == ST_RUNNING);
		}

  void on_btnTerminate_clicked() { close(); }

  void on_btnConfig_clicked();

    //void on_btnReset_clicked() {
    //  Reset();
    //}
    void on_btnStart_clicked(bool cont = false) {
      m_runstarttime = 0.0;
      StartRun(cont ? "Continued" : "");
      EmitStatus("RUN", to_string(m_runnumber));
      emit StatusChanged("EVENT", "0");
      emit StatusChanged("TRIG", "0");
      emit StatusChanged("PARTICLES", "0");
      emit StatusChanged("RATE", "");
      emit StatusChanged("MEANRATE", "");
      SetState(ST_RUNNING);
    }
    void on_btnStop_clicked() {
      StopRun();
      EmitStatus("RUN", "(" + to_string(m_runnumber) + ")");
      SetState(ST_READY);
    }
    void on_btnLog_clicked() {
      std::string msg = txtLogmsg->displayText().toStdString();
      EUDAQ_USER(msg);
    }
    void timer() {
      if (!m_stopping) {
        if (m_event_number >= m_max_event and m_event_number > 0){
          EUDAQ_INFO("Max event number reached! Stopping run!");
          std::cout << '\a';
          std::cout << '\a';
          on_btnStop_clicked();
        }
        if (m_runsizelimit >= 1024 && m_filebytes >= m_runsizelimit) {
          EUDAQ_INFO("File limit reached: " + to_string(m_filebytes) + " > " + to_string(m_runsizelimit));
          eudaq::mSleep(1000);
          StopRun(false);
          eudaq::mSleep(8000);
          while (m_producerbusy) {
            eudaq::mSleep(50);
          }
          on_btnStart_clicked(true);
        } else if (dostatus) {
          GetStatus();
        }
      }
    }
    void ChangeStatus(const QString & name, const QString & value) {
      status_t::iterator i = m_status.find(name.toStdString());
      if (i != m_status.end()) {
        i->second->setText(value);
      }
    }
	void btnLogSetStatusSlot(bool status){

		 btnLog->setEnabled(status);
	}
	
signals:
    void StatusChanged(const QString &, const QString &);
	void btnLogSetStatus(bool status);
	void SetState(int status);
  private:
    RunControlModel m_run;
    RunConnectionDelegate m_delegate;
    QTimer m_statustimer;
    typedef std::map<std::string, QLabel *> status_t;
    status_t m_status;
    double m_runstarttime;
    int64_t m_filebytes;
    uint32_t m_event_number;
    bool dostatus;
    std::vector<std::pair<std::string, std::string>> status_labels;
    std::vector<std::pair<std::string, std::string>> scaler_labels;
};
