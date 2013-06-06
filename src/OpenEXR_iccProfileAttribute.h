//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#ifndef OPENEXR_ICC_PROFILE_ATTRIBUTE_H
#define OPENEXR_ICC_PROFILE_ATTRIBUTE_H


#include <ImfAttribute.h>
#include <ImfHeader.h>

class iccProfileAttribute : public Imf::Attribute
{
  public:
	iccProfileAttribute();
	iccProfileAttribute(void *profile, size_t size);
	virtual ~iccProfileAttribute();
	
	const void *value(size_t &size) const;
	
	virtual const char *typeName() const;
	static const char *staticTypeName();
	
	virtual Imf::Attribute *copy() const;
	
	virtual void writeValueTo(Imf::OStream &os, int version) const;
	virtual void readValueFrom (Imf::IStream &is, int size, int version);
	
	virtual void copyValueFrom(const Imf::Attribute &other);
	
	static Imf::Attribute *makeNewAttribute();
	static void registerAttributeType();
	
  protected:

  private:
	void *_profile;
	size_t _size;
};


void addICCprofile(Imf::Header &header, const iccProfileAttribute &value);
void addICCprofile(Imf::Header &header, void *profile, size_t size);
bool hasICCprofile(const Imf::Header &header);
const iccProfileAttribute & ICCprofileAttribute(const Imf::Header &header);
iccProfileAttribute & ICCprofileAttribute(Imf::Header &header);


#endif // OPENEXR_ICC_PROFILE_ATTRIBUTE_H