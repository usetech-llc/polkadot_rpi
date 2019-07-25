#include <gtk/gtk.h>
#include <iostream>
#include <polkacpp/polkacpp.h>
#include <thread>

#include "private_key.h"

using namespace std;

string addressFrom = "5ECcjykmdAQK71qHBCkEWpWkoMJY6NXvpdKy8UeMx16q5gFr";
string addressTo = "5FpxCaAovn3t2sTsbBeT5pWTj2rg392E8QoduwAyENcPrKht";

GtkWidget *MainWindow;
GtkWidget *background;
GtkWidget *fixedPanel;
GtkWidget *button;
GtkWidget *addressLabelFrom;
GtkWidget *addressLabelTo;
GtkWidget *balanceLabel;
GtkWidget *progressLabel;

thread *workerThread = nullptr;
IApplication *api;

void UpdateBalance(double balance) {
    char balanceStr[1024];

    if (balance >= 1)
        sprintf(balanceStr, "<span size='xx-large' color='#CCCCCC'><b>Wallet Balance:</b> %f DOT</span>", balance);
    else
        sprintf(balanceStr, "<span size='xx-large' color='#CCCCCC'><b>Wallet Balance:</b> %f mDOT</span>",
                balance * 1000);
    gtk_label_set_markup(GTK_LABEL(balanceLabel), balanceStr);
    gtk_widget_show(balanceLabel);
}

void UpdateProgress(string msg) {
    char progressStr[1024];
    sprintf(progressStr, "<span size='medium' color='#CCCCCC'>Status: %s</span>", msg.c_str());
    gtk_label_set_markup(GTK_LABEL(progressLabel), progressStr);
    gtk_widget_show(progressLabel);
}

void SubscribeBalance() {
    // Load balance from Polkadot API
    api->subscribeBalance(addressFrom, [&](uint128 balance) {
        balance /= 1000000;
        long balLong = (long)balance;

        // Show balance in the UI
        UpdateBalance((double)balLong / 1000000000.);
    });
}

void SendDotsThread() {
    UpdateProgress("Transferring DOTs - Sending transaction...");
    api->signAndSendTransfer(addressFrom, senderPrivateKeyStr, addressTo, 1000000000000, [&](string result) {
        if (result == "ready")
            UpdateProgress("Transferring DOTs - Registered in Network...");
        if (result == "finalized") {
            UpdateProgress("Transferring DOTs - Transaction Mined!");
        }
    });
}

gboolean button_click_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (workerThread) {
        workerThread->join();
        delete workerThread;
    }

    workerThread = new thread(SendDotsThread);
    return FALSE; // Return false so event will be called again
}

void CreateButton() {
    button = gtk_button_new_with_label("Send DOTs");

    g_signal_connect(button, "clicked", G_CALLBACK(button_click_event), (gpointer)fixedPanel);

    /* This packs the button into the fixed containers window. */
    gtk_fixed_put(GTK_FIXED(fixedPanel), button, 50, 270);

    /* The final step is to display this newly created widget. */
    gtk_widget_show(button);
}

void CreateLabels() {
    addressLabelFrom = gtk_label_new(NULL);
    addressLabelTo = gtk_label_new(NULL);
    balanceLabel = gtk_label_new(NULL);
    progressLabel = gtk_label_new(NULL);

    char addrLbl[1024];
    sprintf(addrLbl, "<span size='large' color='#CCCCCC'><b>From Wallet Address:</b> %s</span>", addressFrom.c_str());
    gtk_label_set_markup(GTK_LABEL(addressLabelFrom), addrLbl);

    sprintf(addrLbl, "<span size='large' color='#CCCCCC'><b>To Receiving Address:</b> %s</span>", addressTo.c_str());
    gtk_label_set_markup(GTK_LABEL(addressLabelTo), addrLbl);
    gtk_label_set_markup(GTK_LABEL(balanceLabel),
                         "<span size='xx-large' color='#CCCCCC'><b>Wallet Balance:</b> ?</span>");
    gtk_label_set_markup(GTK_LABEL(progressLabel), "<span size='meduim' color='#CCCCCC'>Status: Ready</span>");

    gtk_fixed_put(GTK_FIXED(fixedPanel), addressLabelFrom, 50, 100);
    gtk_fixed_put(GTK_FIXED(fixedPanel), addressLabelTo, 50, 120);
    gtk_fixed_put(GTK_FIXED(fixedPanel), balanceLabel, 50, 170);
    gtk_fixed_put(GTK_FIXED(fixedPanel), progressLabel, 50, 250);

    gtk_widget_show(addressLabelFrom);
    gtk_widget_show(addressLabelTo);
    gtk_widget_show(balanceLabel);
    gtk_widget_show(progressLabel);
}

int main(int argc, char *argv[]) {
    //---------------------------------
    //----- CREATE THE GTK WINDOW -----
    //---------------------------------

    gtk_init(&argc, &argv);

    MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL); // GTK_WINDOW_TOPLEVEL = Has a titlebar and border,
                                                      // managed by the window manager.
    gtk_window_set_title(GTK_WINDOW(MainWindow), "Polkadot GUI for C++ API");
    gtk_container_set_border_width(GTK_CONTAINER(MainWindow), 1);
    gtk_window_set_default_size(GTK_WINDOW(MainWindow), 400,
                                300); // Size of the the client area (excluding the additional areas
                                      // provided by the window manager)
    gtk_window_set_position(GTK_WINDOW(MainWindow), GTK_WIN_POS_CENTER);

    /* Create a Fixed Container */
    fixedPanel = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(MainWindow), fixedPanel);
    gtk_widget_show(fixedPanel);

    // background
    background = gtk_image_new_from_file("background.png");
    gtk_fixed_put(GTK_FIXED(fixedPanel), background, 0, 0);

    CreateButton();
    CreateLabels();

    gtk_widget_show_all(MainWindow);

    // Close the application if the x button is pressed if ALT+F4 is used
    g_signal_connect(G_OBJECT(MainWindow), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Connect API
    api = polkadot::api::getInstance()->app();
    api->connect("");

    SubscribeBalance();
    UpdateProgress("Ready");

    //----- ENTER THE GTK MAIN LOOP -----
    gtk_main(); // Enter the GTK+ main loop until the application closes.

    if (workerThread) {
        workerThread->join();
        delete workerThread;
    }

    api->unsubscribeBalance(addressFrom);
    api->disconnect();

    return 0;
}
