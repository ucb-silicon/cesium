#ifndef __SLIB_CITY_ATTRIBUTE_H__
#define __SLIB_CITY_ATTRIBUTE_H__

#include "../common/types.h"
#include "../registration/registration.h"
#include "../string/stringutils.h"
#include "../util/census.h"
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <fstream>
#include <map>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <string>
#include <vector>

DECLARE_string(hostname);
DECLARE_string(database);
DECLARE_string(database_user);
DECLARE_string(database_password);

namespace slib {
  namespace city {
    class CensusBlock;
    class CensusTract;
  }
}

namespace slib {
  namespace city {

    class Attribute {
    public:
      Attribute() {}
      Attribute(const LatLon& location, const double& weight);
      virtual bool InitializeFromLine(const std::string& line) = 0;

      inline LatLon GetLocation() const {
	return _location;
      }

      inline double GetWeight() const {
	return _weight;
      }

      inline void SetWeight(const double& weight) {
	_weight = weight;
      }

    protected:
      LatLon _location;
      double _weight;
    };

    class CensusAttribute {
    public:
      CensusAttribute() {}
      CensusAttribute(const CensusAttribute& original) {
	_value = original.GetValue();
	_block = original.GetBlock();
	_tract = original.GetTract();
      }

      virtual bool Initialize(const sql::ResultSet& record) = 0;
      virtual bool InitializeWithBlock(const sql::ResultSet& record);
      virtual bool InitializeWithTract(const sql::ResultSet& record);

      virtual const slib::ShapefilePolygon GetGeometry() const;
      virtual CensusAttribute* clone() = 0;

      CensusBlock* GetBlock() const;
      CensusTract* GetTract() const;

      // For backwards compatibility.
      inline double GetWeight() const {
	return _value;
      }

      inline double GetValue() const {
	return _value;
      }

      static CensusAttribute* CreateByName(const std::string& name);
      static CensusAttribute* Register(const std::string& name, CensusAttribute* attribute);

    protected:
      CensusBlock* _block;
      CensusTract* _tract;
      double _value;

      static std::map<std::string, CensusAttribute*> _registry;
    };

    template <class T>
    class Attributes {
    public:
      Attributes() {}
      Attributes(const std::string& filename) {
	this->ReadAttributesFromFile(filename);
      }

      inline void Filter() {
	T::Filter(_attributes);
      }

      static inline void Filter(std::vector<T*>* attributes) {
	T::Filter(attributes);
      }
      
      inline const std::vector<T*> GetAttributes() const {
	return _attributes;
      }

      inline const T* GetAttribute(const int32& index) const {
	return _attributes[index];
      }

    protected:
      void ReadAttributesFromFile(const std::string& filename) {
	std::ifstream filestr(filename.c_str());
	if (!filestr.good()) {
	  LOG(WARNING) << "Could not open file: " << filename;
	  return;
	}
	std::string line;
	while (filestr.good()) {
	  getline(filestr, line);
	  T* attribute = new T;
	  if (attribute) {
	    if (attribute->InitializeFromLine(line)) {
	      _attributes.push_back(attribute);
	    }
	  } 
	}
	filestr.close();      
      }
      std::vector<T*> _attributes;
    };

    class CensusAttributes {
    public:
      CensusAttributes() {}
      CensusAttributes(const std::string& table, 
		       const std::string& type,
		       const std::string& table_index = "",
		       const std::string& geometry_table = "") {
	ReadAttributesFromDatabase(table, type, table_index, geometry_table);
      }

      inline const std::vector<CensusAttribute*> GetAttributes() const {
	return _attributes;
      }

      inline const CensusAttribute* GetAttribute(const int32& index) const {
	return _attributes[index];
      }

    protected:
      std::vector<CensusAttribute*> _attributes;

      void ReadAttributesFromDatabase(const std::string& table, 
				      const std::string& type,
				      const std::string& table_index,
				      const std::string& geometry_table) {
	try {
	  sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
	  LOG(INFO) << "Connecting to database (" << FLAGS_database << ") at " << FLAGS_hostname;
	  sql::Connection* con = driver->connect("tcp://" + FLAGS_hostname, 
						 FLAGS_database_user, 
						 FLAGS_database_password);
	  if (!con) {
	    LOG(ERROR) << "Could not connect to database (" << FLAGS_database << ") at " << FLAGS_hostname;
	    return;
	  }
	  sql::Statement* stmt = con->createStatement();
	  if (!stmt) {
	    return;
	  }
	  stmt->execute("USE " + FLAGS_database);
	  sql::ResultSet* records = NULL;
	  if (table_index != "" && geometry_table != "") {
	    records = stmt->executeQuery(" SELECT * FROM " + table +
					 " JOIN " + geometry_table + " ON " + 
					 geometry_table + "." + table_index + "=" + table + "." + table_index +
					 " WHERE 1");
	  } else {
	    records = stmt->executeQuery("SELECT * FROM " + table + " WHERE 1");
	  }
	  if (!records) {
	    return;
	  }
	  while (records->next()) {
	    CensusAttribute* attribute = CensusAttribute::CreateByName(type);
	    if (attribute) {
	      if (geometry_table == "blocks" && attribute->InitializeWithBlock(*records)) {
		this->_attributes.push_back(attribute);
	      } else if (geometry_table == "tracts" && attribute->InitializeWithTract(*records)) {
		this->_attributes.push_back(attribute);
	      } else if (attribute->Initialize(*records)) {
		this->_attributes.push_back(attribute);
	      }
	    }
	  }
	  
	  delete con;
	  delete stmt;
	  delete records;
	} catch (sql::SQLException e) {
	  LOG(ERROR) << "Error loading attributes: " << e.getErrorCode();
	}
      }
    };

#define DEFINE_CENSUS_ATTRIBUTE(TYPE)			\
    virtual CensusAttribute* clone() { return new TYPE(); }
#define REGISTER_CENSUS_ATTRIBUTE(TYPE)					\
    CensusAttribute* TYPE##_dummy = CensusAttribute::Register(#TYPE, new TYPE());

  }  // namespace city
}  // namespace slib

#endif
