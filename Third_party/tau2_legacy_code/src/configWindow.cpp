#include "configWindow.h"

#ifdef GTK3_FOUND
void ConfigWindow::createGUI(){

      GtkBuilder *builder;
      GObject *window;
      GObject *button;
      GObject *cbBox;
      GObject *bAdj;
      GObject *cAdj;
      GObject* adj;

    builder = gtk_builder_new ();
    if(!gtk_builder_add_from_file (builder, "builder.ui", NULL)){
        std::cerr << "builder could not be created" << std::endl;
        return;
    }

    window = gtk_builder_get_object (builder, "window");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    button = gtk_builder_get_object (builder, "ffcButton");
    g_signal_connect (button, "clicked", G_CALLBACK (doFFC), NULL);

    cbBox = gtk_builder_get_object (builder, "agcType");
    int mode = getAGCMode();
    switch(mode){
        case 5:
            gtk_combo_box_set_active(GTK_COMBO_BOX(cbBox),4);break;
        case 9:
            gtk_combo_box_set_active(GTK_COMBO_BOX(cbBox),5);break;
        case 10:
            gtk_combo_box_set_active(GTK_COMBO_BOX(cbBox),6);break;
        default:
            gtk_combo_box_set_active(GTK_COMBO_BOX(cbBox),mode);break;
    }
    g_signal_connect (cbBox, "changed", G_CALLBACK (gcb_AGCMode),NULL);

    bAdj = gtk_builder_get_object (builder, "brightness-adjustment");
    std::cout << "brightness: " << getBrightness() << std::endl;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(bAdj),getBrightness());
    g_signal_connect (bAdj, "value_changed", G_CALLBACK (gcb_brightness), NULL);

    cAdj = gtk_builder_get_object (builder, "contrast-adjustment");
	std::cout << "contrast: " << getContrast() << std::endl;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(cAdj),getContrast());
    g_signal_connect (cAdj, "value_changed", G_CALLBACK (gcb_contrast), NULL);

    adj = gtk_builder_get_object (builder, "dde-adjustment");
	std::cout << "DDE: " << getDDE() << std::endl;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(adj),(int)getDDE());
    g_signal_connect (adj, "value_changed", G_CALLBACK (gcb_DDE), NULL);

    adj = gtk_builder_get_object (builder, "ace-adjustment");
    gtk_adjustment_set_page_increment(GTK_ADJUSTMENT(adj),1);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(adj),getACE());
    g_signal_connect (adj, "value_changed", G_CALLBACK (gcb_ACE), NULL);

    adj = gtk_builder_get_object (builder, "plateau-adjustment");
    gtk_adjustment_set_value(GTK_ADJUSTMENT(adj),getPlateau());
    g_signal_connect (adj, "value_changed", G_CALLBACK (gcb_Plateau), NULL);

    adj = gtk_builder_get_object (builder, "maxgain-adjustment");
    gtk_adjustment_set_value(GTK_ADJUSTMENT(adj),getMaxGain());
    g_signal_connect (adj, "value_changed", G_CALLBACK (gcb_MaxGain), NULL);

    adj = gtk_builder_get_object (builder, "itt-adjustment");
    gtk_adjustment_set_value(GTK_ADJUSTMENT(adj),getITT());
    g_signal_connect (adj, "value_changed", G_CALLBACK (gcb_ITT), NULL);

    adj = gtk_builder_get_object (builder, "agcf-adjustment");
    gtk_adjustment_set_value(GTK_ADJUSTMENT(adj),getAGCFilter());
    g_signal_connect (adj, "value_changed", G_CALLBACK (gcb_AGCFilter), NULL);

    adj = gtk_builder_get_object (builder, "tail-adjustment");
    gtk_adjustment_set_value(GTK_ADJUSTMENT(adj),getTailRejection()/10.0);
    g_signal_connect (adj, "value_changed", G_CALLBACK (gcb_TailRejection), NULL);

    button = gtk_builder_get_object (builder, "save");
    g_signal_connect (button, "clicked", G_CALLBACK (saveSettings), NULL);
    button = gtk_builder_get_object (builder, "reset");
    g_signal_connect (button, "clicked", G_CALLBACK (factoryReset), NULL);
}

#endif


CameraTau2 ConfigWindow::m_cam;
unsigned short ConfigWindow::m_contrast,ConfigWindow::m_brightness;
int ConfigWindow::m_slider_contrast, ConfigWindow::m_slider_brightness;
