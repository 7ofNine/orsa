#include <QAction>
#include <QApplication>
#include <QTabWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QStringListModel>
#include <QListView>
#include <orsaQt/debug.h>
#include <QFont>

#include <curl/curl.h>

int main(int argc, char ** argv) {
    
    QApplication app(argc, argv);
    
    {
#warning does this work on winxp and macosx
        // app font, try to set it to "fixed font"
        QFont font;
        font.setStyleHint(QFont::TypeWriter);
        font.setFamily("FreeMono");
        app.setFont(font);
    }
    
    // orsaQt::Debug::instance()->initTimer();
    //
    orsa::Debug::instance()->initTimer();
    
    QMainWindow * mainWindow = new QMainWindow;
    
    {
        QMenu * objectMenu = mainWindow->menuBar()->addMenu("Object");
        objectMenu->addAction("Reload NEOCP");
        
        mainWindow->menuBar()->addAction("Preferences");
        
        mainWindow->menuBar()->addAction("About");
    }
    
    QTabWidget * mainTab = new QTabWidget(mainWindow);    
    
    mainWindow->setCentralWidget(mainTab);
    
    QStringList NEOCPlist;
    
    {
        // cURL test

        CURL *curl;
        CURLcode res;

        char outputFileName[1024];
        sprintf(outputFileName,".web.tmp");
        
        FILE * fp = fopen(outputFileName,"w");
        
        curl = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL,"http://www.minorplanetcenter.net/iau/NEO/ToConfirm.html");
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            res = curl_easy_perform(curl);
            /* always cleanup */ 
            curl_easy_cleanup(curl);
        }
        
        fclose(fp);
        
        // re-open read-only to parse
        fp = fopen(outputFileName,"r");
        char line[4096];
        char s_input[1024], s_checkbox[1024], s_name[1024], s_value[1024];
        while (fgets(line,1024,fp)) {
            if (4 == sscanf(line,"%s %s %s %s",s_input,s_checkbox,s_name,s_value)) {
                if ( (std::string(s_input) == std::string("<input")) && 
                     (std::string(s_checkbox) == std::string("type=\"checkbox\"")) &&
                     (std::string(s_name) == std::string("name=\"obj\"")) ) {
                    // ORSA_DEBUG("good line: [%s]",line);
                    char designation[1024];
                    if (1 == sscanf(s_value,"VALUE=\"%s\">",designation)) {
                        if (strlen(designation)>=2) {
                            // remove trailing "> that its TWO characters!
                            designation[strlen(designation)-2] = '\0';
                            // if (strlen(designation)==7) {
                            ORSA_DEBUG("designation: [%s]",designation);
                            NEOCPlist << designation;
                            // }
                        } else {
                            ORSA_DEBUG("designation name too short...");
                        }
                    }
                }
            }
        }
        fclose(fp);
        
    }
    
    {
        QStringListModel *model = new QStringListModel();
        model->setStringList(NEOCPlist);

        
        
        QListView * listView = new QListView(mainTab);
        listView->setModel(model);
        
        mainTab->addTab(listView,"NEOCP");
        
        mainTab->addTab(new QListView(mainTab),"MPEC"); // maybe use obscode?
        
        mainTab->addTab(new QListView(mainTab),"local"); // maybe use obscode?
    }
    
    mainWindow->show();
    
    app.connect(&app,
                SIGNAL(lastWindowClosed()), 
                &app, 
                SLOT(quit()));
    
    return app.exec();
}
