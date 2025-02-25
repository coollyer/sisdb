### 一、What is sisdb?

---

sisdb is a fully open-source time series data set written in ANSI C language, which supports local files, networks, memory, and data persistence.
sisdb has the characteristics of simple deployment, low latency, rich data formats, and multi-table data streaming playback.
It was initially designed to meet the needs of collecting high-traffic data of securities market quotations, storing and accessing factor data, and providing data support for artificial intelligence algorithms.

### 二、Features of sisdb

1. It adopts the WebSocket protocol format and can conveniently provide data support for the Web front end.
2. Due to the different requirements for data formats in the production environment and the development environment, for developers, the best situation is to use the JSON format for data interaction in the development stage; and use the compressed binary data stream in the production environment. This ensures the efficiency of development and enables a smooth transition to a safe and reliable production environment. It mainly supports data storage formats such as CSV format, SNO format, MAP format, and LOG format.

3. With the increasing demand for cross-platform development, the current cross-system front ends are usually mainly WEB applications. sisdb directly provides the standard WebSocket communication protocol to provide data support, giving it an inherent advantage in providing data support for cross-platform clients.
4. sisdb is a high-performance, secure, and convenient time series structured database specifically developed for the securities system. It optimizes the storage and access of securities market quotations and trading data, and not only supports binary structured data streams but also supports the input and output of JSON and CSV data formats, facilitating users' data analysis.
5. sisdb provides plug-in-based function expansion, which is convenient for secondary development and easily realizes large system-level programs.
6. It supports single-key single-table data acquisition and multi-key multi-table data subscription.


### 三、Getting Started with sisdb

#### 3.1 Download and Install sisdb

```shell
    git clone  https://github.com/coollyer/sisdb.git
    cd sisdb/bin
    make
```

#### 3.2 Run sisdb

```shell
    ./sisdb -f sisdb-server.conf 
```

#### 3.3 Run the client for sample data

Simple access
```shell
    cd sisdb/client/web/search.html
```

Complex examples of accessing data
```shell
    cd sisdb/
    git clone  https://github.com/coollyer/wclient.git
    cd wclient
    npm run start
```

### 四、Explanation of sisdb

#### 4.1 Directory Explanation

   bin - Output files, configuration files, etc.

   client - Examples of clients accessing the database

   mock - Location for storing sample data files
 
   sdk - Third-party libraries

   src - Source code

   workers - Directory for plugins

#### 4.2 Main Configuration File

sisdb-server.conf -- The configuration file used when operating as an independent service.
For details of the main configuration file, see the instructions in the configuration file.

#### 4.3 Explanation of Data Structure Fields

   | Field Name | Data Type | Length | Quantity | Decimal Point | Field Constraints | 
   | :---    | :----: | :----: | :----: | :----: | :----: |
   | String  | Refer to the data type definition | Field length | Defaults to 1 | Defaults to 0 | I for index, O for unique | 

#### 4.4 Explanation of Data Types
| Data Type Indicator | Type Description | Byte Description | Remarks |
| :---:     | :----: | :----: | :---- |
|  T   |Milliseconds      | 8       | 1630906080840  |
|  S   |Seconds        |4、8     | 1630906080|
|  I   |Integer      |1、2、4、8| -1234|
|  U   |Unsigned Integer |1、2、4、8| 1234|
|  C   |Character Type     |1..N     | SH600600 |
|  F   |Floating Point Number	     |4、8     | 1234.56|
|  P   |Integer      |4、8     | 1234.56|
   
