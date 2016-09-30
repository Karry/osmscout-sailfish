
import QtQuick 2.0
import Sailfish.Silica 1.0

Image{
    id: poiIcon

    property string poiType: 'unknown'
    property string unknownTypeIcon: 'marker'

    property variant iconMapping: {
                'address': 'marker',

                'amenity_atm': 'bank',
                'amenity_bank_building': 'bank',
                'amenity_bank': 'bank',
                'amenity_cafe_building': 'cafe',
                'amenity_cafe': 'cafe',
                'amenity_fast_food_building': 'fast-food',
                'amenity_fast_food': 'fast-food',
                'amenity_fuel_building': 'fuel',
                'amenity_fuel': 'fuel',
                'amenity_grave_yard': 'religious-christian',
                'amenity_hospital_building': 'hospital',
                'amenity_hospital': 'hospital',
                'amenity_kindergarten_building': 'playground',
                'amenity_kindergarten': 'playground',
                'amenity_parking_building': 'parking-garage',
                'amenity_parking': 'parking',
                'amenity_pharmacy': 'pharmacy',
                'amenity_post_office_building': 'post',
                'amenity_post_office': 'post',
                'amenity_restaurant_building': 'restaurant',
                'amenity_restaurant': 'restaurant',
                'amenity_school_building': 'school',
                'amenity_school': 'school',

                'railway_station': 'rail',
                'aeroway_aerodrome': 'airport',
                'aeroway_terminal': 'airport',

                'leisure_building': 'building',
                'leisure_sports_centre': 'socker',
                'leisure_stadium': 'stadium',
                'leisure_golf_course': 'golf',
                'leisure_water_park': 'swimming',
                'leisure_swimming_pool': 'swimming',
                'leisure_marina': 'ferry',
                'leisure_park': 'park',

                'tourism_aquarium': 'circle-stroked',
                'tourism_attraction_building': 'circle-stroked',
                'tourism_attraction': 'circle-stroked',
                'tourism_artwork': 'circle-stroked',
                'tourism_camp_site': 'circle-stroked',
                'tourism_caravan_site': 'circle-stroked',
                'tourism_picnic_site': 'circle-stroked',
                'tourism_theme_park': 'circle-stroked',
                'tourism_zoo': 'zoo',
                'tourism_chalet_building': 'shelter',
                'tourism_chalet': 'shelter',
                'tourism_guest_house_building': 'circle-stroked',
                'tourism_guest_house': 'circle-stroked',
                'tourism_hostel_building': 'lodging',
                'tourism_hostel': 'lodging',
                'tourism_hotel_building': 'lodging',
                'tourism_hotel': 'lodging',
                'tourism_information_building': 'information',
                'tourism_information': 'information',
                'tourism_motel_building': 'lodging',
                'tourism_motel': 'lodging',
                'tourism_museum_building': 'museum',
                'tourism_museum': 'museum',
                'tourism_building': 'town-hall',
                'tourism': 'circle-stroked',

                'historic_castle_building': 'castle',
                'historic_castle': 'castle',
                'historic_manor_building': 'town-hall',
                'historic_manor': 'town-hall',
                'historic_monument_building': 'landmark',
                'historic_monument': 'landmark',
                'historic_memorial_building': 'landmark',
                'historic_memorial': 'landmark',
                'historic_ruins_building': 'castle',
                'historic_ruins': 'castle',
                'historic_archaeological_site': 'square-stroked',
                'historic_battlefield': 'square-stroked',
                'historic_wreck': 'square-stroked',
                'historic_building': 'town-hall',
                'historic': 'square-stroked',

                'place_locality': 'square-stroked'
        }

    function iconUrl(icon){
        return '../../poi-icons/' + icon + '.svg';
    }

    function typeIcon(type){
      if (typeof iconMapping[type] === 'undefined'){
          console.log("Can't find icon for type " + type);
          return iconUrl(unknownTypeIcon);
      }
      return iconUrl(iconMapping[type]);
    }
    
    source: typeIcon(poiType)

    fillMode: Image.PreserveAspectFit
    horizontalAlignment: Image.AlignHCenter
    verticalAlignment: Image.AlignVCenter

    sourceSize.width: width
    sourceSize.height: height
}

