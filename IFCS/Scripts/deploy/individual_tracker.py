from statistics import mean


class LabelData:
    def __init__(self, cat_id: int, x: float, y: float, w: float, h: float, conf: float):
        self.cat_id = cat_id
        self.x = x
        self.y = y
        self.w = w
        self.h = h
        self.conf = conf

    def get_distance(self, other, width, height):
        """
        the real width, height size in pixel is actaully of no meaning...
        since you can adjust resolution as you want.
        ..
        however, the w-h ratio means a lot
        """
        return (((self.x - other.x) * width) ** 2 + ((self.y - other.y) * height) ** 2) ** 0.5

    def get_approx_body_size(self, width, height):
        """
        return body length and height!

        *** the bigger one is body length!! ***
        """
        bl = max(self.w * width, self.y * height)
        bh = min(self.w * width, self.y * height)
        return bl, bh


class Individual:
    def __init__(self, frame_num, label_data):
        self.info = {}  # dict[int, LabelData]
        self.is_completed = False
        self.has_picked = False
        if frame_num != -1 and label_data != -1:  # trick to make costructor overload?
            self.info[frame_num] = label_data

    def get_enter_frame_num(self):
        return sorted(self.info)[0]

    def get_leave_frame_num(self):
        return sorted(self.info)[-1]

    ## TODO: apply new other exclusion logic to cpp
    def get_name(self, camera_info):
        max_conf = 0
        others_name = camera_info["OtherCategoryName"]
        max_cat = others_name
        for _, label in self.info.items():
            if label.conf < max_conf:
                continue
            category_name = camera_info["Categories"][label.cat_id]            
            if category_name != others_name:            
                max_conf = label.conf
                max_cat = category_name
        return max_cat

    def add_info(self, new_frame_num: int, new_label_data: LabelData):
        self.info[new_frame_num] = new_label_data


def label_file_to_list(file_path):
    out = []
    with open(file_path, "r") as f:
        content = f.read()
    for l in content.split("\n")[:-1]:
        d = l.split(" ")
        out.append(
            LabelData(
                int(d[0]),
                float(d[1]),
                float(d[2]),
                float(d[3]),
                float(d[4]),
                float(d[5]),
            )
        )
    return out


def is_size_similar(label1, label2, frame_diff, camera_info):
    if not camera_info["ShouldEnableBodySizeThreshold"]:
        return True
    bl1, bh1 = label1.get_approx_body_size(16, 9)
    s1 = bl1 * bh1
    bl2, bh2 = label2.get_approx_body_size(16, 9)
    s2 = bl2 * bh2
    th = camera_info["BodySizeThreshold"]
    return (
        s1 < s2 * (1 + th) ** (frame_diff + 1)
        and s1 > s2 * (1 - th) ** (frame_diff + 1)
        or s2 < s1 * (1 + th) ** (frame_diff + 1)
        and s2 > s1 * (1 - th) ** (frame_diff + 1)
    )


def is_distance_acceptable(label1, label2, frame_diff, camera_info):
    if not camera_info["ShouldEnableSpeedThreshold"]:
        return True
    distance = label1.get_distance(label2, 16, 9)
    return distance / frame_diff / (16**2 + 9**2) ** 0.5 < camera_info["SpeedThreshold"]


def track_individual(camera_info, loaded_labels) -> list[Individual]:
    individual_data = []
    min_fishway_pos = min(camera_info["FishwayStartEnd"])
    max_fishway_pos = max(camera_info["FishwayStartEnd"])
    fishway_start, fishway_end = camera_info["FishwayStartEnd"]

    loaded_labels = dict(sorted(loaded_labels.items()))  # sort dict...
    for k, v in loaded_labels.items():
        print(k)
        print(v)
        print("***")
    raise
    temp_indivial_data = []  # list[Individual] # list of individual
    for frame_num, labels in loaded_labels.items():
        for label in labels:
            is_in_fishway = False
            is_in_leaving_area = False

            ## TODO: add another enter zone check if here!  (dev in IFCS test, and then here!)
            ## just another paramer to control is_in_leaving_area? should be suffice?

            if camera_info["IsPassVertical"]:
                is_in_fishway = (label.y >= min_fishway_pos) and (label.y <= max_fishway_pos)
                is_in_leaving_area = fishway_end if fishway_start > fishway_end else label.y > fishway_end
            else:
                is_in_fishway = (label.x >= min_fishway_pos) and (label.x <= max_fishway_pos)
                is_in_leaving_area = label.x < fishway_end if fishway_start > fishway_end else label.x > fishway_end

            if is_in_fishway:
                if len(temp_indivial_data) == 0:
                    temp_indivial_data.append(Individual(frame_num, label))
                    temp_indivial_data[0].has_picked = True
                else:
                    closest_idx = -1
                    closest_distance = 999999
                    i = 0
                    for data in temp_indivial_data:
                        if data.has_picked:
                            continue
                        # last_frame_num = sorted(data.info)[-1]
                        # print(last_frame_num)
                        last_frame_num = max(data.info.keys())
                        # print(last_frame_num)
                        # print("....")
                        # print(f"frame num: {frame_num} vs last frame num {last_frame_num}")
                        last_label = data.info[last_frame_num]
                        distance = last_label.get_distance(label, 16, 9)
                        if (
                            is_size_similar(last_label, label, frame_num - last_frame_num, camera_info)
                            and is_distance_acceptable(last_label, label, frame_num - last_frame_num, camera_info)
                            and distance <= closest_distance
                        ):
                            closest_idx = i
                            closest_distance = distance
                        i += 1
                    if closest_idx == -1:
                        temp_indivial_data.append(Individual(frame_num, label))
                        temp_indivial_data[0].has_picked = True
                    else:
                        temp_indivial_data[closest_idx].add_info(frame_num, label)
                        temp_indivial_data[closest_idx].has_picked = True
            elif is_in_leaving_area:
                closest_idx = -1
                closest_distance = 999999
                i = 0
                for data in temp_indivial_data:
                    # last_frame_num = sorted(data.info)[-1]
                    last_frame_num = max(data.info.keys())
                    last_label = data.info[last_frame_num]
                    distance = last_label.get_distance(label, 16, 9)
                    if (
                        is_size_similar(last_label, label, frame_num - last_frame_num, camera_info)
                        and is_distance_acceptable(last_label, label, frame_num - last_frame_num, camera_info)
                        and distance <= closest_distance
                    ):
                        closest_idx = i
                        closest_distance = distance
                    i += 1
                if closest_idx != -1:
                    temp_indivial_data[closest_idx].has_picked = True
                    temp_indivial_data[closest_idx].is_completed = True

        # how did python handle ref? I need ref, and don't know it is or not?
        temp_indivial_data2 = []
        for data in temp_indivial_data:
            data.has_picked = False
            temp_indivial_data2.append(data)
        # temp_indivial_data = temp_indivial_data2.copy()

        for data in temp_indivial_data2:
            if len(data.info) == 1:
                continue
            last_frame_num = max(data.info.keys())
            if last_frame_num + camera_info["FrameBufferSize"] < frame_num or data.is_completed:
                individual_data.append(data)

        temp_indivial_data = []
        for data in temp_indivial_data2:
            if max(data.info.keys()) + camera_info["FrameBufferSize"] < frame_num or data.is_completed:
                continue
            temp_indivial_data.append(data)

        # for data in temp_indivial_data:
        #     data.has_picked = False
        #     if len(data.info) == 1:
        #         continue
        #     last_frame_num = sorted(data.info)[-1]
        #     if last_frame_num + camera_info["FrameBufferSize"] < frame_num or data.is_completed:
        #         individual_data.append(data)
        #
        # temp_indivial_data = [
        #     data
        #     for data in temp_indivial_data
        #     if not data.is_completed and sorted(data.info)[-1] + camera_info["FrameBufferSize"] >= frame_num
        # ]
    return individual_data
