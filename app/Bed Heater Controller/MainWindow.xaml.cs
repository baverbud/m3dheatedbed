using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Bed_Heater_Controller
{

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {

        String SettingsFile;
        Settings Settings;
        object comPortLock = new object();
        SerialPort comPort;
        Thread ReadThread;

        Thread QueryTempThread;

        public MainWindow()
        {
            InitializeComponent();


            List<String> PortNames = new List<String>(System.IO.Ports.SerialPort.GetPortNames());
            comPortBox.ItemsSource = PortNames;

            SettingsFile = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + "\\BedHeaterController\\BedHeaterController.settings";
            Settings = Settings.Load(SettingsFile);

            // Select previously selected options, if any
            foreach (String name in PortNames)
            {
                if (name.CompareTo(Settings.ComPort ?? "") == 0)
                {
                    comPortBox.SelectedItem = name;
                }
            }

            connectBtn.Click += OnConnect;
            sendBtn.Click += OnSend;
            comPortBox.DropDownOpened += OnComPortBoxExpand;
            offBtn.Click += OnTurnOff;

            UpdateState();
        }

        protected void UpdateState()
        {
            if(Connected)
            {
                connectBtn.Content = "Disconnect";
                comPortBox.IsEnabled = false;
                newTempBox.IsEnabled = true;
                sendBtn.IsEnabled = true;
                offBtn.IsEnabled = true;
                currentTempLabel.Content = "??";
                targetTempLabel.Content = "??";
            } else
            {
                connectBtn.Content = "Connect";
                comPortBox.IsEnabled = true;
                newTempBox.IsEnabled = false;
                sendBtn.IsEnabled = false;
                offBtn.IsEnabled = false;
            }
        }

        bool Connected = false;
        public void OnConnect(object sender, EventArgs ea)
        {
            
            if (Connected)
            {
                try
                {
                    lock (comPortLock)
                    {
                        comPort.Close();
                        Connected = false;
                        UpdateState();
                    }
                    ReadThread.Join();
                    ReadThread = null;
                    QueryTempThread.Join();
                    QueryTempThread = null;

                    comPort = null;
                }
                catch (IOException e)
                {

                }
            }
            else
            {
                try
                {
                    String selected = (String)comPortBox.SelectedItem;
                    if (selected == null)
                        throw new Exception("No com port selected");

                    lock(comPortLock)
                    {
                        comPort = new SerialPort(selected, 57600);
                        comPort.Open();
                        comPort.NewLine = "\r\n";
                        comPort.ReadTimeout = 500;
                        comPort.WriteTimeout = 500;

                        Connected = true;
                        UpdateState();
                    }

                    // Store com port selection on successful connect
                    Settings.Save(SettingsFile);

                    ReadThread = new Thread(new ThreadStart(RunReadThread));
                    ReadThread.Start();

                    QueryTempThread = new Thread(new ThreadStart(RunQueryTempThread));
                    QueryTempThread.Start();
                }
                catch (IOException e)
                {
                    if (comPort != null && comPort.IsOpen)
                        comPort.Close();
                    comPort = null;
                    MessageBox.Show(e.Message);
                }
                catch (Exception e)
                {
                    MessageBox.Show(e.Message);
                }
            }
        }

        public void OnComPortBoxExpand(object sender, EventArgs ea)
        {
            String selected = (String)comPortBox.SelectedItem;
            // Refresh list of port names
            List<String> PortNames = new List<String>(System.IO.Ports.SerialPort.GetPortNames());
            comPortBox.ItemsSource = PortNames;

            // Check if selected is in the new list, and select it
            foreach (String name in PortNames)
            {
                if (name.CompareTo(selected ?? "") == 0)
                {
                    comPortBox.SelectedItem = name;
                }
            }
        }

        public void OnSend(object sender, EventArgs ea)
        {
            lock(comPortLock)
            {
                if (Connected)
                {
                    double temp = Double.Parse(newTempBox.Text) * 100.0f;
                    String cmd = $"AT+ SetTemp {temp}";
                    comPort.WriteLine(cmd);
                }
            }
        }

        public void OnTurnOff(object sender, EventArgs ea)
        {
            lock(comPortLock)
            {
                if (Connected)
                {
                    String cmd = "AT+ TurnOff";
                    comPort.WriteLine(cmd);
                }
            }
        }

        public void OnMessage(String msg)
        {
            System.Diagnostics.Debug.WriteLine(msg);
            char[] separators = { ' ' };
            string[] words = msg.Split(separators);
            if (words.Length != 3)
                return;

            try {
                Dispatcher.Invoke((Action)delegate ()
                {
                    if (msg.Contains("AT- SetTempErr"))
                    {
                        MessageBox.Show($"Temp set error: {words[2]}");
                    }
                    else if (msg.Contains("AT- ActualTemp"))
                    {
                        double temp = Double.Parse(words[2]) / 100.0;
                        currentTempLabel.Content = $"{temp}";
                    }
                    else if (msg.Contains("AT- TargetTemp"))
                    {
                        double temp = Double.Parse(words[2]) / 100.0;
                        targetTempLabel.Content = $"{temp}";
                    }
                    else
                    {
                        MessageBox.Show($"Unknown message: {msg}");
                    }
                });
            } catch(TaskCanceledException)
            {

            }
        }

        public void RunReadThread()
        {
            try {
                while (Connected)
                {
                    // Don't lock here, only one thread reading.
                    String line = comPort.ReadLine();
                    OnMessage(line);
                }
            } catch(Exception ex)
            {

            }
        }

        public void RunQueryTempThread()
        {
            try
            {
                
                while (Connected)
                {
                    lock (comPortLock)
                    {
                        String msg = "AT+ GetActualTemp";
                        comPort.WriteLine(msg);


                        msg = "AT+ GetTargetTemp";
                        comPort.WriteLine(msg);

                        Thread.Sleep(250);
                    }
                }
            } catch(Exception e)
            {

            }
        }
    }
}
