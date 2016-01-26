/* +---------------------------------------------------------------------------+
   |                     Mobile Robot Programming Toolkit (MRPT)               |
   |                          http://www.mrpt.org/                             |
   |                                                                           |
   | Copyright (c) 2005-2015, Individual contributors, see AUTHORS file        |
   | See: http://www.mrpt.org/Authors - All rights reserved.                   |
   | Released under BSD License. See details in http://www.mrpt.org/License    |
   +---------------------------------------------------------------------------+ */

#ifndef CGPSInterface_H
#define CGPSInterface_H

#include <mrpt/obs/CObservationGPS.h>
#include <mrpt/poses/CPoint3D.h>
#include <mrpt/hwdrivers/CSerialPort.h>
#include <mrpt/utils/CDebugOutputCapable.h>
#include <mrpt/utils/CFileOutputStream.h>
#include <mrpt/utils/TEnumType.h>
#include <mrpt/hwdrivers/CGenericSensor.h>
#include <mrpt/utils/circular_buffer.h>
#include <mrpt/obs/obs_frwds.h>

namespace mrpt
{
	namespace hwdrivers
	{
		/** A class capable of reading GPS/GNSS/GNSS+IMU receiver data, from a serial port or from any input stream, 
		  *  and \b parsing the ASCII/binary stream into indivual messages \b stored in mrpt::obs::CObservationGPS objects.
		  *
		  * Typical input streams are serial ports or raw GPS log files.
		  *
		  * The parsers in the enum type CGPSInterface::PARSERS are supported as parameter `parser` in the 
		  * configuration file below or in method CGPSInterface::setParser():
		  *  - `NMEA` (NMEA 0183, ASCII messages): Default parser. Supported frames: GGA, RMC.
		  *  - `NOVATEL_OEM6` (Novatel OEM6, binary frames): Supported frames: XXX
		  *
		  *  \code
		  *  PARAMETERS IN THE ".INI"-LIKE CONFIGURATION STRINGS:
		  * -------------------------------------------------------
		  * [supplied_section_name]
		  *
		  *  # Serial port configuration:
		  *  COM_port_WIN = COM3
		  *  COM_port_LIN = ttyUSB0
		  *  baudRate     = 4800   // The baudrate of the communications (typ. 4800 or 9600 bauds)
		  *
		  *  # Select a parser for GNSS data:
		  *  # Up-to-date list of supported parsers available in http://reference.mrpt.org/devel/classmrpt_1_1hwdrivers_1_1_c_g_p_s_interface.html
		  *  parser =  NMEA
		  *
		  *  # If uncommented and non-empty, raw binary/ascii data received from the serial port will be also dumped 
		  *  # into a file named after this prefix, plus date/time and extension `.gps`.
		  *  #raw_dump_file_prefix = RAWGPS
		  *
		  *  # 3D position (and orientation, for GNSS+IMUs) of the sensed point (antenna phase center) relative to the vehicle/robot frame:
		  *  pose_x       = 0      // (meters)
		  *  pose_y       = 0
		  *  pose_z       = 0
		  *
		  *  # Optional: initial commands to be sent to the GNSS receiver to set it up.
		  *
		  *
		  *  # The following parameters are *DEPRECATED, DO NOT USE*. They are kept for backwards-compatibility only.
		  *  #customInit   = JAVAD
		  *  #JAVAD_rtk_src_port=/dev/ser/b
		  *  #JAVAD_rtk_src_baud=9600
		  *  #JAVAD_rtk_format=cmr
		  *  \endcode
		  *
		  * - customInit: Custom commands to send, depending on the sensor. Valid values are:
		  *		- "": Empty string
		  *		- "JAVAD": JAVAD or TopCon devices. Extra initialization commands will be sent.
		  *		- "TopCon": A synonymous with "JAVAD".
		  *
		  *  The next picture summarizes existing MRPT classes related to GPS / GNSS devices (CGPSInterface, CNTRIPEmitter, CGPS_NTRIP):
		  *
		  *  <div align=center> <img src="mrpt_gps_classes_usage.png"> </div>
		  *
		  * <b>VERSIONS HISTORY:</b>
		  * - 9/JUN/2006: First version (JLBC)
		  * - 4/JUN/2008: Added virtual methods for device-specific initialization commands.
		  * - 10/JUN/2008: Converted into CGenericSensor class (there are no inhirited classes anymore).
		  * - 7/DEC/2012: Added public static method to parse NMEA strings.
		  * - 17/JUN/2014: Added GGA feedback.
		  * - 24/JAN/2015: API changed for MTPT 1.4.0
		  *
		  *  \note Verbose debug info will be dumped to cout if the environment variable "MRPT_HWDRIVERS_VERBOSE" is set to "1", or if you call CGenericSensor::enableVerbose(true)
		  *  \note 
		  *  \note <b>[API changed in MRPT 1.4.0]</b> mrpt::hwdrivers::CGPSInterface API clean-up and made more generic so any stream can be used to parse GNSS messages, not only serial ports.
		  *
		  * \sa CGPS_NTRIP, CNTRIPEmitter, mrpt::obs::CObservationGPS 
		  * \ingroup mrpt_hwdrivers_grp
		  */
		class HWDRIVERS_IMPEXP CGPSInterface : public utils::CDebugOutputCapable, public CGenericSensor
		{
			DEFINE_GENERIC_SENSOR(CGPSInterface)

		public:
			/** Read about parser selection in the documentation for CGPSInterface */
			enum PARSERS
			{
				NMEA         = 0,
				NOVATEL_OEM6
			};

			CGPSInterface(); //!< Default ctor
			virtual ~CGPSInterface();  //!< Dtor

			void  doProcess(); // See docs in parent class

			bool  isGPS_connected(); //!< Returns true if communications work, i.e. if some message has been received.
			bool  isGPS_signalAcquired(); //!< Returns true if the last message from the GPS indicates that the signal from sats has been acquired.

			/** \name Set-up and configuration 
			  * @{ */
			void  setSerialPortName(const std::string &COM_port);  //!< Set the serial port to use (COM1, ttyUSB0, etc).
			std::string getSerialPortName() const;  //!< Get the serial port to use (COM1, ttyUSB0, etc).

			void  setParser(PARSERS parser);  //!< Select the parser for incomming data, among the options enumerated in \a CGPSInterface
			PARSERS getParser() const;

			inline void setExternCOM( CSerialPort *outPort, mrpt::synch::CCriticalSection *csOutPort )
			{ m_out_COM = outPort; m_cs_out_COM = csOutPort; }

			/** @} */

			inline bool isAIMConfigured() { return m_AIMConfigured; }

			/** Parses one line of NMEA data from a GPS receiver, and writes the recognized fields (if any) into an observation object.
			  * Recognized frame types are: "GGA" and "RMC".
			  * \return true if some new data field has been correctly parsed and inserted into out_obs
			  */
			static bool parse_NMEA(const std::string &cmd_line, mrpt::obs::CObservationGPS &out_obs, const bool verbose=false);

			/** Gets the latest GGA command or an empty string if no newer GGA command was received since the last call to this method.
			  * \param[in] reset If set to true, will empty the GGA cache so next calls will return an empty string if no new frame is received.
			  */
			std::string getLastGGA(bool reset=true);

		protected:
			/** Implements custom messages to be sent to the GPS unit just after connection and before normal use.
			  *  Returns false or raise an exception if something goes wrong.
			  */
			bool OnConnectionEstablished();

			CSerialPort		m_COM;

			// MAR'11 -------------------------------------
			CSerialPort		                *m_out_COM;
			mrpt::synch::CCriticalSection   *m_cs_out_COM;
			// --------------------------------------------

			poses::CPose3D m_sensorPose;
			std::string		m_customInit;

			/** See the class documentation at the top for expected parameters */
			void  loadConfig_sensorSpecific(
				const mrpt::utils::CConfigFileBase &configSource,
				const std::string	  &iniSection );

			/** If not empty, will send a cmd "set,/par/pos/pd/port,...". Example value: "/dev/ser/b" */
			void setJAVAD_rtk_src_port( const std::string &s) { m_JAVAD_rtk_src_port = s; }

			/** Only used when "m_JAVAD_rtk_src_port" is not empty */
			void setJAVAD_rtk_src_baud(unsigned int baud) { m_JAVAD_rtk_src_baud = baud; }

			/** Only used when "m_JAVAD_rtk_src_port" is not empty: format of RTK corrections: "cmr", "rtcm", "rtcm3", etc. */
			void setJAVAD_rtk_format(const std::string &s) {m_JAVAD_rtk_format=s;}

			/** Set Advanced Input Mode for the primary port.
				This can be used to send RTK corrections to the device using the same port that it's used for the commands.
				The RTK correction stream must be re-packaged into a special frame with prefix ">>" */
			bool setJAVAD_AIM_mode();

			/** Unset Advanced Input Mode for the primary port and use it only as a command port. */
			bool unsetJAVAD_AIM_mode();

			// MAR'11 -------------------------------------
			inline bool useExternCOM() const { return (m_out_COM!=NULL); }
			// --------------------------------------------

		private:
			mrpt::utils::circular_buffer<uint8_t> m_rx_buffer; //!< Auxiliary buffer for readings
			PARSERS      m_parser;
			std::string  m_raw_dump_file_prefix;
			std::string  m_COMname;
			int          m_COMbauds;
			bool         m_GPS_comsWork;
			bool         m_GPS_signalAcquired;

			mrpt::utils::CFileOutputStream  m_raw_output_file;

			std::string		m_JAVAD_rtk_src_port; 	//!< If not empty, will send a cmd "set,/par/pos/pd/port,...". Example value: "/dev/ser/b"
			unsigned int	m_JAVAD_rtk_src_baud; 	//!< Only used when "m_JAVAD_rtk_src_port" is not empty
			std::string		m_JAVAD_rtk_format; 	//!< Only used when "m_JAVAD_rtk_src_port" is not empty: format of RTK corrections: "cmr", "rtcm", "rtcm3", etc.

			// MAR'11 -----------------------------------------
			bool            m_useAIMMode;           //!< Use this mode for receive RTK corrections from a external source through the primary port
			// ------------------------------------------------
			mrpt::system::TTimeStamp      m_last_timestamp;

			// MAR'11 -----------------------------------------
			bool            m_AIMConfigured;        //!< Indicates if the AIM has been properly set up.
			double          m_data_period;          //!< The period in seconds which the data should be provided by the GPS
			// ------------------------------------------------

			/** Returns true if the COM port is already open, or try to open it in other case.
			  * \return true if everything goes OK, or false if there are problems opening the port.
			  */
			bool  tryToOpenTheCOM();

			void  processBuffer(); //!< Process data in "m_buffer" to extract GPS messages, and remove them from the buffer.

			void  implement_parser_NMEA();
			void  implement_parser_NOVATEL_OEM6();

			void  processGPSstring( const std::string &s); //!< Process a complete string from the GPS:

			/* A private copy of the last received gps datum */
			mrpt::obs::CObservationGPS	            m_latestGPS_data;
			mrpt::obs::CObservationGPS::TUTCTime   m_last_UTC_time;
			
			std::string   m_last_GGA; //!< Used in getLastGGA()
			
			void JAVAD_sendMessage(const char*str, bool waitForAnswer = true); //!< Private auxiliary method. Raises exception on error.

		}; // end class
	} // end namespace
	// Specializations MUST occur at the same namespace:
	namespace utils
	{
		template <>
		struct TEnumTypeFiller<hwdrivers::CGPSInterface::PARSERS>
		{
			typedef hwdrivers::CGPSInterface::PARSERS enum_t;
			static void fill(bimap<enum_t,std::string>  &m_map)
			{
				m_map.insert(hwdrivers::CGPSInterface::NMEA,          "NMEA");
				m_map.insert(hwdrivers::CGPSInterface::NOVATEL_OEM6,  "NOVATEL_OEM6");
			}
		};
	}
} // end namespace

#endif
