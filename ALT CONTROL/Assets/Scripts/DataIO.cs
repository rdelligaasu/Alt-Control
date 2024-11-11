using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Collections;
using UnityEngine;

public class DataIO : MonoBehaviour
{
    private UdpClient client;
    private IPEndPoint remoteEndPoint;
    private bool isGrounded = true;

    // IMU data variables
    private float iFromIMU, jFromIMU, kFromIMU;
    private float iOffset, jOffset, kOffset;

    // Movement parameters
    public GameObject objectToTrack;
    public float moveForce = 5.0f;
    public float deadZoneThreshold = 0.1f; // Dead zone to prevent minor drift

    private Rigidbody rb;

    void Start()
    {
        client = new UdpClient(4211);
        client.BeginReceive(new AsyncCallback(ReceiveCallback), null);
        remoteEndPoint = new IPEndPoint(IPAddress.Parse("192.168.1.76"), 4211);
        rb = objectToTrack.GetComponent<Rigidbody>();

        iOffset = iFromIMU;
        jOffset = jFromIMU;
        kOffset = kFromIMU;
    }

    void Update()
    {
        // Manual recalibration
        if (Input.GetKeyDown(KeyCode.R))
        {
            iOffset = iFromIMU;
            jOffset = jFromIMU;
            kOffset = kFromIMU;
        }

        // Apply horizontal movement with dead zone
        float horizontalMovement = Mathf.Abs(iFromIMU - iOffset) > deadZoneThreshold ? (iFromIMU - iOffset) / 100.0f : 0;
        rb.AddForce(new Vector3(horizontalMovement * moveForce, 0, 0), ForceMode.Force);

        // Apply forward/backward movement with dead zone
        float verticalMovement = Mathf.Abs(jFromIMU - jOffset) > deadZoneThreshold ? (jFromIMU - jOffset) / 100.0f : 0;
        rb.AddForce(new Vector3(0, 0, verticalMovement * moveForce), ForceMode.Force);
    }

    private void ReceiveCallback(IAsyncResult ar)
    {
        byte[] receivedBytes = client.EndReceive(ar, ref remoteEndPoint);
        string data = Encoding.ASCII.GetString(receivedBytes);

        string[] values = data.Split(',');
        foreach (string value in values)
        {
            if (value.Contains("i"))
                iFromIMU = float.Parse(value.Split(':')[1].Trim());
            else if (value.Contains("j"))
                jFromIMU = float.Parse(value.Split(':')[1].Trim());
            else if (value.Contains("k"))
                kFromIMU = float.Parse(value.Split(':')[1].Trim());
        }

        client.BeginReceive(new AsyncCallback(ReceiveCallback), null);
    }

    private void OnCollisionEnter(Collision collision)
    {
        if (collision.gameObject.CompareTag("Ground"))
        {
            isGrounded = true;
        }
    }

    void OnApplicationQuit()
    {
        client.Close();
    }
}
