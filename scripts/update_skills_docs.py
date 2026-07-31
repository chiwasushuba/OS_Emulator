import os
import re
import pathlib

def parse_skill_md(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return None

    # Extract frontmatter
    match = re.match(r'^---\s*\n(.*?)\n---\s*\n(.*)$', content, re.DOTALL)
    if not match:
        print(f"Warning: No valid frontmatter found in {file_path}")
        return None
    
    frontmatter = match.group(1)
    body = match.group(2).strip()

    name = None
    description = None
    for line in frontmatter.split('\n'):
        if line.startswith('name:'):
            name = line.split('name:', 1)[1].strip()
        elif line.startswith('description:'):
            description = line.split('description:', 1)[1].strip()
            
    # Remove redundant header from body if it exists
    # e.g., if body starts with "# CPU Skill" or "# cpu", remove it
    body_lines = body.split('\n')
    if body_lines and body_lines[0].startswith('# ') and name and name.lower() in body_lines[0].lower():
        body = '\n'.join(body_lines[1:]).strip()
    
    return name, description, body

def main():
    root_dir = pathlib.Path(__file__).parent.parent
    github_skills_dir = root_dir / '.github' / 'skills'
    docs_skills_dir = root_dir / '.docs' / 'skills'

    docs_skills_dir.mkdir(parents=True, exist_ok=True)

    skills_data = []

    if github_skills_dir.exists():
        for skill_folder in github_skills_dir.iterdir():
            if skill_folder.is_dir():
                skill_md_path = skill_folder / 'SKILL.md'
                if skill_md_path.exists():
                    parsed = parse_skill_md(skill_md_path)
                    if parsed:
                        name, description, body = parsed
                        skills_data.append({
                            'folder': skill_folder.name,
                            'name': name or skill_folder.name,
                            'description': description or '',
                            'body': body
                        })

    # Generate or update .docs/skills/<name>.md
    generated_files = []
    for skill in skills_data:
        md_name = f"{skill['folder']}.md"
        generated_files.append(md_name)
        out_path = docs_skills_dir / md_name
        
        display_name = skill['name'].upper() if len(skill['name']) <= 3 else skill['name'].title()
        
        with open(out_path, 'w', encoding='utf-8') as f:
            f.write(f"<!-- This file is auto-generated. Do not edit manually. -->\n")
            f.write(f"# {display_name} Skill\n\n")
            f.write(f"**Name**: {skill['name']}\n\n")
            f.write(f"**Description**: {skill['description']}\n\n")
            f.write(f"{skill['body']}\n")

    # Update README.md
    manual_files = []
    if docs_skills_dir.exists():
        for f in docs_skills_dir.iterdir():
            if f.is_file() and f.suffix == '.md' and f.name != 'README.md':
                if f.name not in generated_files:
                    manual_files.append(f.name)
    
    readme_path = docs_skills_dir / 'README.md'
    with open(readme_path, 'w', encoding='utf-8') as f:
        f.write("# Project Skills\n\n")
        f.write("This document describes the specialized skills available for agents in this project.\n\n")
        f.write("## Available Skills\n\n")
        
        # Combine automated and manual files for a unified list, or separate them.
        # The prompt asked to maintain an accurate list with links of all available skills, handling manual files.
        all_skills = []
        for skill in skills_data:
            all_skills.append((skill['name'], f"{skill['folder']}.md", skill['description']))
            
        for m in manual_files:
            m_title = m.replace('-', ' ').replace('_', ' ').replace('.md', '').title()
            all_skills.append((m_title, m, "Manual documentation"))
            
        all_skills.sort(key=lambda x: x[0].lower())
        
        for name, link, desc in all_skills:
            if desc and desc != "Manual documentation":
                f.write(f"- [{name}]({link}) - {desc}\n")
            else:
                f.write(f"- [{name}]({link})\n")
                
        f.write("\n## Using Skills\n\n")
        f.write("Skills are specialized folders containing instructions, scripts, and resources. When a routing role identifies a relevant skill for a task, it can read its `SKILL.md` instructions and leverage any helper scripts or resources provided to complete the task effectively.\n")

if __name__ == '__main__':
    main()
