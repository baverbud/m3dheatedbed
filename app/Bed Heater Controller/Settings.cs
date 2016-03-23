using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace Bed_Heater_Controller
{
    [Serializable()]
    class Settings : INotifyPropertyChanged
    {
        [field: NonSerialized()]
        public event PropertyChangedEventHandler PropertyChanged;

        protected void OnPropertyChanged(String name)
        {
            if (PropertyChanged != null) PropertyChanged(this, new PropertyChangedEventArgs(name));
        }


        protected String mComPort;
        public String ComPort
        {
            get { return mComPort; }
            set { mComPort = value; OnPropertyChanged("ComPort"); }
        }
        
        public void Save(String file)
        {
            try
            {
                Directory.CreateDirectory(new FileInfo(file).Directory.FullName);
                Stream stream = File.Create(file);

                BinaryFormatter serializer = new BinaryFormatter();
                serializer.Serialize(stream, this);
                stream.Close();
            }
            catch (DirectoryNotFoundException e)
            {
                MessageBox.Show(e.Message);
            }
        }

        public static Settings Load(String file)
        {
            Settings s;
            if (File.Exists(file))
            {
                Stream stream = File.OpenRead(file);
                BinaryFormatter deserializer = new BinaryFormatter();
                try
                {
                    s = (Settings)deserializer.Deserialize(stream);
                }
                catch (SerializationException)
                {
                    s = new Settings();
                }
                stream.Close();
            }
            else
            {
                s = new Settings();
            }
            return s;
        }
    }
}
