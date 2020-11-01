using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using UnityEngine;
using Valve.VR;


public class TrackerPos : MonoBehaviour
{

    public SteamVR_TrackedController controller;
    //public SteamVR_TrackedObject controller;

    // Variable for file name
    public string DataType;
    private int TrainingNum = 1;
    private bool PressedFlag = false;
    
    StringBuilder sb = new StringBuilder();


    // Variable for data index, data position
    private int DataIdx = 0;
    private Vector3 tracker_pos;
    private List<string[]> rowData = new List<string[]>();


    void Start()
    {
        /*
        string filename = "Train_" + DataType + ".csv";
        string filePath = getPath(filename);
        StreamWriter outStream = new StreamWriter(filePath, true);
        string[] initial_List = new string[] {"Index", "X", "Y", "Z"};
        StringBuilder sb = new StringBuilder();
        sb.AppendLine(string.Join(",", initial_List));
        outStream.Write(sb);
        //outStream.Close();
        Debug.Log(initial_List);
        Debug.Log(sb);
        */
    }

    // Update is called once per frame
    void Update()
    {
        //var device = SteamVR_Controller.Input((int)controller.trigger.index);
        //if (device.GetPressDown(SteamVR_Controller.ButtonMask.Trigger))
        if(!PressedFlag && controller.triggerPressed)
        {
            PressedFlag = true;
            string[] initial_List = new string[] {"Index", "X", "Y", "Z"};
            sb.AppendLine(string.Join(",", initial_List));
        }
        if(controller.triggerPressed){
            //Debug.Log("Pressed");
            tracker_pos = this.transform.position;
            Debug.Log(tracker_pos);
            Save(sb);
        }
        if(PressedFlag && !controller.triggerPressed){
            StringBuilder sb = new StringBuilder();
            PressedFlag = false;
            TrainingNum += 1;
        }
    }

    void Save(StringBuilder sb)
    {

        string[] rowDataTemp = new string[4];
        rowDataTemp[0] = DataIdx.ToString(); // Data Index
        rowDataTemp[1] = tracker_pos.x.ToString();
        rowDataTemp[2] = tracker_pos.y.ToString();
        rowDataTemp[3] = tracker_pos.z.ToString();
        rowData.Add(rowDataTemp);
        DataIdx += 1;

        string[][] output = new string[rowData.Count][];

        for (int i = 0; i < output.Length; i++)
        {
            output[i] = rowData[i];
        }

        int length = output.GetLength(0);
        string delimiter = ",";

        // StringBuilder sb = new StringBuilder();
        sb.AppendLine(string.Join(delimiter, rowDataTemp));

        //for (int index = 0; index < length; index++)
        //    sb.AppendLine(string.Join(delimiter, output[index]));

        string filename = "U" + user + "_" + DataType + "_" + TrainingNum + ".csv";
        string filePath = getPath(filename);

        StreamWriter outStream = new StreamWriter(filePath, true);
        outStream.Write(sb);
        outStream.Close();

        // StreamWriter outStream = System.IO.File.CreateText(filePath);
        /*if (System.IO.File.Exists(filename))
        {
            using (System.IO.StreamWriter outStream = new System.IO.StreamWriter(@filePath, true))
            {
                outStream.WriteLine(sb);
                outStream.Close();
            }
        }
        else
        {
            StreamWriter outStream = System.IO.File.CreateText(filePath);
            outStream.WriteLine(sb);
            outStream.Close();
        }*/
        
    }

    // Following method is used to retrive the relative path as device platform
    private string getPath(string filename)
    {
        return "D://IDVR_T3_final//TrainingData//" + filename;
        /*
        #if UNITY_EDITOR
        return Application.dataPath +"/CSV/"+"Saved_data.csv";
        #elif UNITY_ANDROID
        return Application.persistentDataPath+"Saved_data.csv";
        #elif UNITY_IPHONE
        return Application.persistentDataPath+"/"+"Saved_data.csv";
        #else
        return Application.dataPath +"/"+"Saved_data.csv";
        #endif
		*/
    }
}
